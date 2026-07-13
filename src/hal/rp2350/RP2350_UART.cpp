#include "RP2350_UART.h"

#if !defined(UNIT_TEST)
void RP2350UART::begin(uint32_t baud) {
    if (_rxPin >= 0) _serial.setRX((uint8_t)_rxPin);
    if (_txPin >= 0) _serial.setTX((uint8_t)_txPin);
    _serial.begin(baud, _config);
}

int RP2350UART::available() {
    return _serial.available();
}

int RP2350UART::read() {
    return _serial.read();
}

size_t RP2350UART::write(uint8_t value) {
    return _serial.write(value);
}

size_t RP2350UART::write(const uint8_t* data, size_t length) {
    return _serial.write(data, length);
}
#endif
