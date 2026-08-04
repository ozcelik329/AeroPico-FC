#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <Arduino.h>

class IHALSPI {
  public:
    virtual ~IHALSPI() {}
    virtual void begin(uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin) = 0;
    virtual void beginTransaction(uint32_t hz) = 0;
    virtual uint8_t transfer(uint8_t value) = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    virtual void endTransaction() = 0;
};

#endif
