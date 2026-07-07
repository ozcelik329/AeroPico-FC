#ifndef RP2350_TIMER_H
#define RP2350_TIMER_H

#include "../HAL_Timer.h"

class RP2350Timer : public IHALTimer {
  public:
    uint32_t micros() const override;
    uint32_t millis() const override;
    void delayMs(uint32_t ms) override;
};

#endif
