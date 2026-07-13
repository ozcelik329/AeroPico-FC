#include "RP2350_I2C.h"

#include "hardware/gpio.h"

RP2350I2C::RP2350I2C(i2c_inst_t* instance)
    : _instance(instance) {}

void RP2350I2C::init(uint8_t sdaPin, uint8_t sclPin, uint32_t baudHz) {
    i2c_init(_instance, baudHz);
    gpio_set_function(sdaPin, GPIO_FUNC_I2C);
    gpio_set_function(sclPin, GPIO_FUNC_I2C);
    gpio_pull_up(sdaPin);
    gpio_pull_up(sclPin);
}

bool RP2350I2C::writeRaw(uint8_t address, const uint8_t* data, size_t length, bool nostop) {
    return i2c_write_blocking(_instance, address, data, length, nostop) == (int)length;
}

bool RP2350I2C::readRaw(uint8_t address, uint8_t* data, size_t length, bool nostop) {
    return i2c_read_blocking(_instance, address, data, length, nostop) == (int)length;
}

bool RP2350I2C::writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
    const uint8_t bytes[2] = {reg, value};
    return writeRaw(address, bytes, sizeof(bytes), false);
}

bool RP2350I2C::readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, size_t length) {
    return writeRaw(address, &reg, 1, true) && readRaw(address, buffer, length, false);
}

void RP2350I2C::setDmaEnabled(bool txEnabled, bool rxEnabled) {
    constexpr uint32_t RX_DMA_ENABLE = 1u << 0;
    constexpr uint32_t TX_DMA_ENABLE = 1u << 1;
    _instance->hw->dma_cr = (txEnabled ? TX_DMA_ENABLE : 0u) | (rxEnabled ? RX_DMA_ENABLE : 0u);
}
