#ifndef PID_H
#define PID_H

#include <Arduino.h>

class PID {
  public:
    PID(float kp, float ki, float kd);
    float __not_in_flash_func(compute)(float setpoint, float measured_value, float dt);
    void reset();
    void setGains(float kp, float ki, float kd);

  private:
    float kp, ki, kd;
    float prev_error;
    float integral;
};

#endif