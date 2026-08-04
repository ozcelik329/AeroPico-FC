#ifndef RP2350_SPI_H
#define RP2350_SPI_H

#include "../HAL_SPI.h"

#if !defined(UNIT_TEST)
#include <SPI.h>

class RP2350SPI : public IHALSPI {
  public:
    explicit RP2350SPI(SPIClassRP2040& spi = SPI) : _spi(spi) {}

    void begin(uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin) override;
    void beginTransaction(uint32_t hz) override;
    uint8_t transfer(uint8_t value) override;
    size_t write(const uint8_t* data, size_t length) override;
    void endTransaction() override;

  private:
    SPIClassRP2040& _spi;
};
#endif

#endif
