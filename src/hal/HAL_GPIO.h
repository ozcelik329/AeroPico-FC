#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <Arduino.h>

enum class HALGpioMode : uint8_t {
    Input = 0,
    Output,
    InputPullup,
    InputPulldown
};

class IHALGPIO {
  public:
    virtual ~IHALGPIO() {}
    virtual void configure(uint8_t pin, HALGpioMode mode) = 0;
    virtual void write(uint8_t pin, bool high) = 0;
    virtual bool read(uint8_t pin) const = 0;
};

#endif
