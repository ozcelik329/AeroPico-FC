#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <Arduino.h>

class IHALADC {
  public:
    virtual ~IHALADC() {}
    virtual void init(uint8_t pin, uint8_t channel) = 0;
    virtual bool readRaw12(uint8_t channel, uint16_t& raw) = 0;
    virtual bool readVoltage(uint8_t channel, float scale, float& voltage) = 0;
};

#endif
