#ifndef PID_H
#define PID_H

#include <Arduino.h>
#include "../config.h"

class PID {
public:
    PID(float kp, float ki, float kd) : _kp(kp), _ki(ki), _kd(kd) {}
    float compute(float target, float current, float dt, bool saturated);
    void reset();

private:
    float _kp, _ki, _kd;
    float _integral, _last_error;
};

#endif