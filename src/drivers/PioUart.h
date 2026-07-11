#ifndef PIO_UART_H
#define PIO_UART_H

#include <Arduino.h>
#include "../hal/HAL_UART.h"

#ifdef UNIT_TEST
class PioUart : public IHALUART {
  public:
    void begin(uint32_t baud) override { (void)baud; }
    void init(uint32_t baud = 57600) { begin(baud); }
    size_t write(const uint8_t* buf, size_t len) override {
        (void)buf;
        bytesWritten += len;
        return len;
    }
    size_t write(uint8_t value) override { return write(&value, 1); }
    int available() override { return 0; }
    int read() override { return -1; }
    void serviceTx() {}
    uint32_t droppedBytes() const { return 0; }
    size_t bytesWritten = 0;
};
#else
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "pio_uart.pio.h"
#include "../config.h"

class PioUart : public IHALUART {
  public:
    static constexpr uint16_t TX_QUEUE_CAPACITY = 512;
    static constexpr uint16_t RX_QUEUE_CAPACITY = 512;

    void begin(uint32_t baud) override;
    void init(uint32_t baud = 57600);
    size_t write(const uint8_t* buf, size_t len) override;
    size_t write(uint8_t value) override { return write(&value, 1); }
    int available() override;
    int read() override;
    void serviceTx();
    void serviceRx();
    void handleIrq();
    uint32_t droppedBytes() const { return _droppedBytes; }
    uint32_t rxDroppedBytes() const { return _rxDroppedBytes; }

  private:
    PIO  _pio = pio1;       // pio0 PWM için kullanılıyor
    uint _sm_tx;
    uint _sm_rx;
    uint _offset_tx;
    uint _offset_rx;
    uint8_t _txQueue[TX_QUEUE_CAPACITY] = {};
    uint8_t _rxQueue[RX_QUEUE_CAPACITY] = {};
    volatile uint16_t _txHead = 0;
    volatile uint16_t _txTail = 0;
    volatile uint16_t _rxHead = 0;
    volatile uint16_t _rxTail = 0;
    uint32_t _droppedBytes = 0;
    uint32_t _rxDroppedBytes = 0;

    void enableTxIrq(bool enabled);
};
#endif

extern PioUart espUart;

#endif
