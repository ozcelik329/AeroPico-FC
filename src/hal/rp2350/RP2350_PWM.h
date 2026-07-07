#ifndef RP2350_PWM_H
#define RP2350_PWM_H

#include "../HAL_PWM.h"

class RP2350PWM : public IHALPWM {
  public:
    void init() override;
    void write(const HALPwmOutputs& outputs) override;
};

#endif
