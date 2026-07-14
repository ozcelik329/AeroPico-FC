#include "Output.h"
#include "pwm.pio.h"
#include <hardware/clocks.h>

static PIO pio = pio0;
static int sm_aileron = -1;
static int sm_elevator = -1;
static int sm_rudder = -1;
static int sm_throttle = -1;
static constexpr uint32_t SERVO_UPDATE_PERIOD_US = 20000U;
static uint32_t lastServoWriteUs = 0;
static bool forceNextServoWrite = true;
static ServoOutputStatus outputStatus = {};
static int pendingThrottle = PWM_MIN;
static int pendingRoll = PWM_NEUTRAL;
static int pendingPitch = PWM_NEUTRAL;
static int pendingYaw = PWM_NEUTRAL;
static uint32_t pendingFrameUs = 0;

// ServoOutput implementation
ServoOutput servoOutput;

// forward declarations for helper functions
static bool initServoSM(uint sm, uint pin, uint offset);
static void writePulse(uint sm, int pulse_us);
static void flushPendingFrame(uint32_t nowUs);
static uint32_t packServoFrameUs(int pulse_us);
static float getOneMicrosecondPioClockDivider();

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
    bool ok = initServoSM((uint)sm_aileron,  PIN_AILERON, offset);
    ok = initServoSM((uint)sm_elevator, PIN_ELEVATOR, offset) && ok;
    ok = initServoSM((uint)sm_rudder,   PIN_RUDDER, offset) && ok;
    ok = initServoSM((uint)sm_throttle, PIN_THROTTLE, offset) && ok;

    if (!ok) {
        return;
    }

    _ready = true;
    forceNextServoWrite = true;
    outputStatus = {};
    pendingThrottle = PWM_MIN;
    pendingRoll = PWM_NEUTRAL;
    pendingPitch = PWM_NEUTRAL;
    pendingYaw = PWM_NEUTRAL;
    pendingFrameUs = micros();
    writePulse((uint)sm_aileron,  PWM_NEUTRAL);
    writePulse((uint)sm_elevator, PWM_NEUTRAL);
    writePulse((uint)sm_rudder,   PWM_NEUTRAL);
    writePulse((uint)sm_throttle, PWM_MIN);
}

void ServoOutput::writeMotors(int throttle, int roll, int pitch, int yaw) {
    if (!_ready) {
        return;
    }

    const uint32_t nowUs = micros();
    pendingThrottle = throttle;
    pendingRoll = roll;
    pendingPitch = pitch;
    pendingYaw = yaw;
    pendingFrameUs = nowUs;

    const bool immediateSafeFrame = throttle <= PWM_MIN
        && roll == PWM_NEUTRAL
        && pitch == PWM_NEUTRAL
        && yaw == PWM_NEUTRAL;
    if (!immediateSafeFrame
        && !forceNextServoWrite
        && (uint32_t)(nowUs - lastServoWriteUs) < SERVO_UPDATE_PERIOD_US) {
        __atomic_add_fetch(&outputStatus.staleSkips, 1U, __ATOMIC_RELAXED);
        return;
    }
    forceNextServoWrite = false;
    lastServoWriteUs = nowUs;

    flushPendingFrame(nowUs);
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

static void flushPendingFrame(uint32_t nowUs) {
    const uint32_t latencyUs = nowUs - pendingFrameUs;
    updateMaxLatency(latencyUs);
    __atomic_store_n(&outputStatus.lastWriteUs, nowUs, __ATOMIC_RELEASE);
    writePulse((uint)sm_aileron,  pendingRoll);
    writePulse((uint)sm_elevator, pendingPitch);
    writePulse((uint)sm_rudder,   pendingYaw);
    writePulse((uint)sm_throttle, pendingThrottle);
}

static void writePulse(uint sm, int pulse_us) {
    if (pio_sm_is_tx_fifo_full(pio, sm)) {
        __atomic_add_fetch(&outputStatus.fifoDrops, 1U, __ATOMIC_RELAXED);
        return;
    }
    pio_sm_put(pio, sm, packServoFrameUs(pulse_us));
    __atomic_add_fetch(&outputStatus.framesWritten, 1U, __ATOMIC_RELAXED);
}

void outputInit() {
    // Backwards-compatible wrapper
    servoOutput.init();
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
