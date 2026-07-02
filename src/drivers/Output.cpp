#include "Output.h"
#include "pwm.pio.h"

static PIO pio = pio0;
static uint sm_aileron, sm_elevator, sm_rudder, sm_throttle;

// ServoOutput implementation
ServoOutput servoOutput;

// forward declarations for helper functions
static void initServoSM(uint sm, uint pin);
static void writePulse(uint sm, int pulse_us);

void ServoOutput::init() {
    sm_aileron  = pio_claim_unused_sm(pio, true);
    sm_elevator = pio_claim_unused_sm(pio, true);
    sm_rudder   = pio_claim_unused_sm(pio, true);
    sm_throttle = pio_claim_unused_sm(pio, true);

    initServoSM(sm_aileron,  PIN_AILERON);
    initServoSM(sm_elevator, PIN_ELEVATOR);
    initServoSM(sm_rudder,   PIN_RUDDER);
    initServoSM(sm_throttle, PIN_THROTTLE);

    writePulse(sm_aileron,  PWM_NEUTRAL);
    writePulse(sm_elevator, PWM_NEUTRAL);
    writePulse(sm_rudder,   PWM_NEUTRAL);
    writePulse(sm_throttle, PWM_MIN);
}

void ServoOutput::writeMotors(int throttle, int roll, int pitch, int yaw) {
    writePulse(sm_aileron,  roll);
    writePulse(sm_elevator, pitch);
    writePulse(sm_rudder,   yaw);
    writePulse(sm_throttle, throttle);
}

void ServoOutput::setServoPulse(void* p, unsigned sm, uint32_t pulse_us) {
    // p is unused here; keep signature compatible
    int clamped = constrain(pulse_us, PWM_MIN, PWM_MAX);
    pio_sm_put(pio, sm, (uint32_t)clamped);
}

static void initServoSM(uint sm, uint pin) {
    uint offset = pio_add_program(pio, &pwm_servo_program);
    pio_sm_config c = pwm_servo_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    // 125MHz / 125 = 1MHz → 1 tick = 1µs
    sm_config_set_clkdiv(&c, 125.0f);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static void writePulse(uint sm, int pulse_us) {
    int clamped = constrain(pulse_us, PWM_MIN, PWM_MAX);
    pio_sm_put(pio, sm, (uint32_t)clamped);
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