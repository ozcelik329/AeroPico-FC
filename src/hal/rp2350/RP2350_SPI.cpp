#include "RP2350_SPI.h"

#if !defined(UNIT_TEST)
void RP2350SPI::begin(uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin) {
    _spi.setSCK(sckPin);
    _spi.setRX(misoPin);
    _spi.setTX(mosiPin);
    _spi.begin();
}

void RP2350SPI::beginTransaction(uint32_t hz) {
    _spi.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
}

uint8_t RP2350SPI::transfer(uint8_t value) {
    return _spi.transfer(value);
}

size_t RP2350SPI::write(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return 0;
    }
    _spi.transfer((void*)data, length);
    return length;
}

void RP2350SPI::endTransaction() {
    _spi.endTransaction();
}
#endif
