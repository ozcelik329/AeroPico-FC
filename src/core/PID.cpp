#include "PID.h"
#include "pico/platform.h"

float __not_in_flash_func(PID::compute)(float target, float current, float dt, bool saturated) {
    float error = target - current;
    
    // Anti-Windup (Risk 3 FIX - Koşullu Entegrasyon)
    if (!saturated) {
        _integral += error * dt;
        _integral = constrain(_integral, -PID_I_LIMIT, PID_I_LIMIT);
    }

    float derivative = (error - _last_error) / dt;
    _last_error = error;

    return (_kp * error) + (_ki * _integral) + (_kd * derivative);
}

void PID::reset() {
    _integral = 0;
    _last_error = 0;
}