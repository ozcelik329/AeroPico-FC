#ifndef PIO_UART_H
#define PIO_UART_H

#include <Arduino.h>

#ifdef UNIT_TEST
class PioUart {
  public:
    void init(uint32_t baud = 57600) { (void)baud; }
    void write(const uint8_t* buf, size_t len) {
        (void)buf;
        bytesWritten += len;
    }
    bool available() { return false; }
    uint8_t read() { return 0; }
    size_t bytesWritten = 0;
};
#else
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pio_uart.pio.h"
#include "../config.h"

class PioUart {
  public:
    void init(uint32_t baud = 57600);
    void write(const uint8_t* buf, size_t len);
    bool available();
    uint8_t read();

  private:
    PIO  _pio = pio1;       // pio0 PWM için kullanılıyor
    uint _sm_tx;
    uint _sm_rx;
    uint _offset_tx;
    uint _offset_rx;
};
#endif

extern PioUart espUart;

#endif
