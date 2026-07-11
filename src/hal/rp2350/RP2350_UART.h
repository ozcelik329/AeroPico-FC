#ifndef RP2350_UART_H
#define RP2350_UART_H

#include "../HAL_UART.h"

#if !defined(UNIT_TEST)
#include <Arduino.h>
#include <SerialUART.h>

class RP2350UART : public IHALUART {
  public:
    explicit RP2350UART(SerialUART& serial, int8_t rxPin = -1, int8_t txPin = -1,
                       uint16_t config = SERIAL_8N1)
        : _serial(serial), _rxPin(rxPin), _txPin(txPin), _config(config) {}

    void begin(uint32_t baud) override;
    int available() override;
    int read() override;
    size_t write(uint8_t value) override;
    size_t write(const uint8_t* data, size_t length) override;

  private:
    SerialUART& _serial;
    int8_t _rxPin;
    int8_t _txPin;
    uint16_t _config;
};
#endif

#endif
