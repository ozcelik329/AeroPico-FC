#include "PID.h"

PID::PID(float p, float i, float d)
    : kp(p), ki(i), kd(d), prev_error(0), integral(0) {}

float __not_in_flash_func(PID::compute)(float setpoint, float measured_value, float dt) {
    float error = setpoint - measured_value;

    float P = kp * error;

    integral += error * dt;
    integral = constrain(integral, -100.0f, 100.0f);
    float I = ki * integral;

    float D = kd * ((error - prev_error) / dt);

    prev_error = error;

    return P + I + D;
}

void PID::reset() {
    integral   = 0.0f;
    prev_error = 0.0f;
}

void PID::setGains(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}