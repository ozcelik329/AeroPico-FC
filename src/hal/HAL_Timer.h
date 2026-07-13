#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <Arduino.h>

class IHALTimer {
  public:
    virtual ~IHALTimer() {}
    virtual uint32_t micros() const = 0;
    virtual uint32_t millis() const = 0;
    virtual void delayMs(uint32_t ms) = 0;
};

#endif
