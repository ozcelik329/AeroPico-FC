#ifndef PID_H
#define PID_H

#include <Arduino.h>

class PID {
  public:
    PID(float kp, float ki, float kd);
    PID(float kp, float ki, float kd, float outputMin, float outputMax, float integralLimit = 100.0f);
    float compute(float setpoint, float measured_value, float dt);
    void reset();
    void setGains(float kp, float ki, float kd);
    void setOutputLimits(float minOutput, float maxOutput);
    void setIntegralLimit(float limit);
    float getIntegral() const;

  private:
    float kp, ki, kd;
    float prev_error;
    float prev_measurement;
    float integral;
    float outputMin;
    float outputMax;
    float integralLimit;
    bool hasPrevMeasurement;
};

#endif
