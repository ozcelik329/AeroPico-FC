#include "Output.h"
#include "pwm.pio.h"
#include <hardware/clocks.h>
#include <pico/time.h>

static PIO pio = pio0;
static int sm_aileron = -1;
static int sm_elevator = -1;
static int sm_rudder = -1;
static int sm_throttle = -1;
static constexpr uint32_t SERVO_UPDATE_PERIOD_US = 20000U;
static uint32_t lastServoWriteUs = 0;
static ServoOutputStatus outputStatus = {};
static repeating_timer servoFrameTimer;
static bool servoFrameTimerActive = false;
static volatile uint32_t pendingThrottleFrame = 0;
static volatile uint32_t pendingRollFrame = 0;
static volatile uint32_t pendingPitchFrame = 0;
static volatile uint32_t pendingYawFrame = 0;
static volatile uint32_t pendingFrameUs = 0;
static volatile bool pendingFrameDirty = false;
static ServoPinConfig servoPins = {
    PIN_AILERON,
    PIN_ELEVATOR,
    PIN_RUDDER,
    PIN_THROTTLE
};

// ServoOutput implementation
ServoOutput servoOutput;

// forward declarations for helper functions
static bool initServoSM(uint sm, uint pin, uint offset);
static void writePulse(uint sm, int pulse_us);
static void writePackedPulse(uint sm, uint32_t packed);
static uint32_t packServoFrameUs(int pulse_us);
static float getOneMicrosecondPioClockDivider();
static void updateMaxLatency(uint32_t latencyUs);
static bool servoFrameTimerCallback(repeating_timer* timer);
static void updatePendingFrame(int throttle, int roll, int pitch, int yaw, uint32_t nowUs);
static bool validServoPin(uint8_t pin);

void ServoOutput::init() {
    _ready = false;

    if (!pio_can_add_program(pio, &pwm_servo_program)) {
        return;
    }

    sm_aileron  = pio_claim_unused_sm(pio, false);
    sm_elevator = pio_claim_unused_sm(pio, false);
    sm_rudder   = pio_claim_unused_sm(pio, false);
    sm_throttle = pio_claim_unused_sm(pio, false);

    if (sm_aileron < 0 || sm_elevator < 0 || sm_rudder < 0 || sm_throttle < 0) {
        return;
    }

    uint offset = pio_add_program(pio, &pwm_servo_program);
    bool ok = initServoSM((uint)sm_aileron,  servoPins.aileron, offset);
    ok = initServoSM((uint)sm_elevator, servoPins.elevator, offset) && ok;
    ok = initServoSM((uint)sm_rudder,   servoPins.rudder, offset) && ok;
    ok = initServoSM((uint)sm_throttle, servoPins.throttle, offset) && ok;

    if (!ok) {
        return;
    }

    _ready = true;
    outputStatus = {};
    const uint32_t nowUs = time_us_32();
    lastServoWriteUs = nowUs;
    updatePendingFrame(PWM_MIN, PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL, nowUs);
    pendingFrameDirty = true;
    serviceFrame();

    if (!servoFrameTimerActive) {
        servoFrameTimerActive = add_repeating_timer_us(
            -(int64_t)SERVO_UPDATE_PERIOD_US,
            servoFrameTimerCallback,
            nullptr,
            &servoFrameTimer
        );
    }
}

void ServoOutput::configurePins(const ServoPinConfig& pins) {
    if (_ready) {
        return;
    }
    if (!validServoPin(pins.aileron) || !validServoPin(pins.elevator) ||
        !validServoPin(pins.rudder) || !validServoPin(pins.throttle)) {
        return;
    }
    if (pins.aileron == pins.elevator || pins.aileron == pins.rudder || pins.aileron == pins.throttle ||
        pins.elevator == pins.rudder || pins.elevator == pins.throttle ||
        pins.rudder == pins.throttle) {
        return;
    }
    servoPins = pins;
}

void ServoOutput::writeMotors(int throttle, int roll, int pitch, int yaw) {
    if (!_ready) {
        return;
    }

    const uint32_t nowUs = time_us_32();
    updatePendingFrame(throttle, roll, pitch, yaw, nowUs);
}

void ServoOutput::serviceFrame() {
    if (!_ready) {
        return;
    }

    const uint32_t nowUs = time_us_32();
    const uint32_t latencyUs = nowUs - __atomic_load_n(&pendingFrameUs, __ATOMIC_ACQUIRE);
    updateMaxLatency(latencyUs);
    __atomic_store_n(&outputStatus.lastWriteUs, nowUs, __ATOMIC_RELEASE);
    __atomic_store_n(&pendingFrameDirty, false, __ATOMIC_RELEASE);
    lastServoWriteUs = nowUs;

    writePackedPulse((uint)sm_aileron,  __atomic_load_n(&pendingRollFrame, __ATOMIC_ACQUIRE));
    writePackedPulse((uint)sm_elevator, __atomic_load_n(&pendingPitchFrame, __ATOMIC_ACQUIRE));
    writePackedPulse((uint)sm_rudder,   __atomic_load_n(&pendingYawFrame, __ATOMIC_ACQUIRE));
    writePackedPulse((uint)sm_throttle, __atomic_load_n(&pendingThrottleFrame, __ATOMIC_ACQUIRE));
}

void ServoOutput::setServoPulse(void* p, unsigned sm, uint32_t pulse_us) {
    PIO targetPio = p ? (PIO)p : pio;
    uint32_t packed = packServoFrameUs((int)pulse_us);
    if (pio_sm_is_tx_fifo_full(targetPio, sm)) {
        __atomic_add_fetch(&outputStatus.fifoDrops, 1U, __ATOMIC_RELAXED);
        return;
    }
    pio_sm_put(targetPio, sm, packed);
    __atomic_add_fetch(&outputStatus.framesWritten, 1U, __ATOMIC_RELAXED);
}

ServoOutputStatus ServoOutput::status() const {
    ServoOutputStatus copy = {};
    copy.framesWritten = __atomic_load_n(&outputStatus.framesWritten, __ATOMIC_ACQUIRE);
    copy.fifoDrops = __atomic_load_n(&outputStatus.fifoDrops, __ATOMIC_ACQUIRE);
    copy.staleSkips = __atomic_load_n(&outputStatus.staleSkips, __ATOMIC_ACQUIRE);
    copy.maxLatencyUs = __atomic_load_n(&outputStatus.maxLatencyUs, __ATOMIC_ACQUIRE);
    copy.lastWriteUs = __atomic_load_n(&outputStatus.lastWriteUs, __ATOMIC_ACQUIRE);
    return copy;
}

static bool initServoSM(uint sm, uint pin, uint offset) {
    pio_sm_config c = pwm_servo_program_get_default_config(offset);
    sm_config_set_set_pins(&c, pin, 1);
    sm_config_set_out_shift(&c, true, false, 32);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    // PIO state machines run from clk_sys. Keep one PIO tick at 1us on both
    // RP2040-class 125MHz and RP2350-class 150MHz default clocks.
    sm_config_set_clkdiv(&c, getOneMicrosecondPioClockDivider());
    pio_sm_clear_fifos(pio, sm);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    return true;
}

static bool validServoPin(uint8_t pin) {
    return pin <= 28 && pin != PIN_SBUS_RX && pin != PIN_SDA && pin != PIN_SCL &&
           pin != PIN_BENCH_ADMIN_GND && pin != PIN_BENCH_ADMIN_SENSE;
}

static float getOneMicrosecondPioClockDivider() {
    return (float)clock_get_hz(clk_sys) / 1000000.0f;
}

static uint32_t packServoFrameUs(int pulse_us) {
    constexpr uint32_t frameUs = 20000U;
    constexpr uint32_t instructionOverheadUs = 5U;
    uint32_t highUs = (uint32_t)constrain(pulse_us, PWM_MIN, PWM_MAX);
    uint32_t highTicks = highUs > 1U ? highUs - 2U : 0U;
    uint32_t lowTicks = frameUs > highUs + instructionOverheadUs
        ? frameUs - highUs - instructionOverheadUs
        : 1U;
    return ((lowTicks & 0xFFFFU) << 16) | (highTicks & 0xFFFFU);
}

static void updateMaxLatency(uint32_t latencyUs) {
    uint32_t current = __atomic_load_n(&outputStatus.maxLatencyUs, __ATOMIC_RELAXED);
    while (latencyUs > current &&
           !__atomic_compare_exchange_n(&outputStatus.maxLatencyUs, &current, latencyUs,
                                        false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
    }
}

static void writePulse(uint sm, int pulse_us) {
    writePackedPulse(sm, packServoFrameUs(pulse_us));
}

static void writePackedPulse(uint sm, uint32_t packed) {
    if (pio_sm_is_tx_fifo_full(pio, sm)) {
        __atomic_add_fetch(&outputStatus.fifoDrops, 1U, __ATOMIC_RELAXED);
        return;
    }
    pio_sm_put(pio, sm, packed);
    __atomic_add_fetch(&outputStatus.framesWritten, 1U, __ATOMIC_RELAXED);
}

static bool servoFrameTimerCallback(repeating_timer* timer) {
    (void)timer;
    servoOutput.serviceFrame();
    return true;
}

static void updatePendingFrame(int throttle, int roll, int pitch, int yaw, uint32_t nowUs) {
    __atomic_store_n(&pendingThrottleFrame, packServoFrameUs(throttle), __ATOMIC_RELEASE);
    __atomic_store_n(&pendingRollFrame, packServoFrameUs(roll), __ATOMIC_RELEASE);
    __atomic_store_n(&pendingPitchFrame, packServoFrameUs(pitch), __ATOMIC_RELEASE);
    __atomic_store_n(&pendingYawFrame, packServoFrameUs(yaw), __ATOMIC_RELEASE);
    __atomic_store_n(&pendingFrameUs, nowUs, __ATOMIC_RELEASE);
    if (__atomic_exchange_n(&pendingFrameDirty, true, __ATOMIC_ACQ_REL)) {
        __atomic_add_fetch(&outputStatus.staleSkips, 1U, __ATOMIC_RELAXED);
    }
}

void outputInit() {
    // Backwards-compatible wrapper
    servoOutput.init();
}

void configureServoOutputPins(const ServoPinConfig& pins) {
    servoOutput.configurePins(pins);
}

void writeMotors(int throttle, int roll, int pitch, int yaw) {
    servoOutput.writeMotors(throttle, roll, pitch, yaw);
}

void setServoPulse(PIO pio, uint sm, uint32_t pulse_us) {
    servoOutput.setServoPulse((void*)pio, sm, pulse_us);
}

ServoOutputStatus getServoOutputStatus() {
    return servoOutput.status();
}
