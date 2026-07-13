#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <Arduino.h>

class IHALI2C {
  public:
    virtual ~IHALI2C() {}
    virtual void init(uint8_t sdaPin, uint8_t sclPin, uint32_t baudHz) = 0;
    virtual bool writeRaw(uint8_t address, const uint8_t* data, size_t length, bool nostop) = 0;
    virtual bool readRaw(uint8_t address, uint8_t* data, size_t length, bool nostop) = 0;
    virtual bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
        const uint8_t bytes[2] = {reg, value};
        return writeRaw(address, bytes, sizeof(bytes), false);
    }
    virtual bool readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, size_t length) {
        return writeRaw(address, &reg, 1, true) && readRaw(address, buffer, length, false);
    }
};

#endif
