#include "PID.h"

PID::PID(float p, float i, float d)
    : PID(p, i, d, -1000000.0f, 1000000.0f, 100.0f) {}

PID::PID(float p, float i, float d, float minOutput, float maxOutput, float iLimit)
    : kp(p),
      ki(i),
      kd(d),
      prev_error(0.0f),
      prev_measurement(0.0f),
      integral(0.0f),
      outputMin(minOutput),
      outputMax(maxOutput),
      integralLimit(iLimit),
      hasPrevMeasurement(false) {}

float __not_in_flash_func(PID::compute)(float setpoint, float measured_value, float dt) {
    if (dt <= 0.0f || dt > 1.0f) {
        dt = 0.001f;
    }

    float error = setpoint - measured_value;

    float P = kp * error;
    float D = 0.0f;
    if (hasPrevMeasurement) {
        D = -kd * ((measured_value - prev_measurement) / dt);
    }

    float candidateIntegral = integral + error * dt;
    candidateIntegral = constrain(candidateIntegral, -integralLimit, integralLimit);

    float output = P + (ki * candidateIntegral) + D;
    bool saturatingHigh = output > outputMax && error > 0.0f;
    bool saturatingLow = output < outputMin && error < 0.0f;

    if (!saturatingHigh && !saturatingLow) {
        integral = candidateIntegral;
    }

    prev_error = error;
    prev_measurement = measured_value;
    hasPrevMeasurement = true;

    output = P + (ki * integral) + D;
    return constrain(output, outputMin, outputMax);
}

void PID::reset() {
    integral   = 0.0f;
    prev_error = 0.0f;
    prev_measurement = 0.0f;
    hasPrevMeasurement = false;
}

void PID::setGains(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}

void PID::setOutputLimits(float minOutput, float maxOutput) {
    outputMin = minOutput;
    outputMax = maxOutput;
}

void PID::setIntegralLimit(float limit) {
    integralLimit = limit < 0.0f ? -limit : limit;
    integral = constrain(integral, -integralLimit, integralLimit);
}

float PID::getIntegral() const {
    return integral;
}
