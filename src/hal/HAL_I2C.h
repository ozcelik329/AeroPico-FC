#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <Arduino.h>

class IHALI2C {
  public:
    virtual ~IHALI2C() {}
    virtual bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) = 0;
    virtual bool readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, size_t length) = 0;
};

#endif
