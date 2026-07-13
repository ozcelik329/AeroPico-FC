#ifndef RP2350_GPIO_H
#define RP2350_GPIO_H

#include "../HAL_GPIO.h"

class RP2350GPIO : public IHALGPIO {
  public:
    void configure(uint8_t pin, HALGpioMode mode) override;
    void write(uint8_t pin, bool high) override;
    bool read(uint8_t pin) const override;
};

#endif
