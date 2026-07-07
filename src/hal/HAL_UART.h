#ifndef HAL_UART_H
#define HAL_UART_H

#include <Arduino.h>

class IHALUART {
  public:
    virtual ~IHALUART() {}
    virtual void begin(uint32_t baud) = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual size_t write(uint8_t value) = 0;
};

#endif
