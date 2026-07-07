#ifndef HAL_PWM_H
#define HAL_PWM_H

#include <Arduino.h>

struct HALPwmOutputs {
    uint16_t throttle;
    uint16_t aileron;
    uint16_t elevator;
    uint16_t rudder;
};

class IHALPWM {
  public:
    virtual ~IHALPWM() {}
    virtual void init() = 0;
    virtual void write(const HALPwmOutputs& outputs) = 0;
};

#endif
