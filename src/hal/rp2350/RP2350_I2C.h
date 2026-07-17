#ifndef RP2350_I2C_H
#define RP2350_I2C_H

#include "../HAL_I2C.h"
#include "hardware/i2c.h"

class RP2350I2C final : public IHALI2C {
  public:
    explicit RP2350I2C(i2c_inst_t* instance = i2c0);

    void init(uint8_t sdaPin, uint8_t sclPin, uint32_t baudHz) override;
    bool writeRaw(uint8_t address, const uint8_t* data, size_t length, bool nostop) override;
    bool readRaw(uint8_t address, uint8_t* data, size_t length, bool nostop) override;
    bool probeAddress(uint8_t address) override;
    bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) override;
    bool readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, size_t length) override;

    i2c_inst_t* native() const { return _instance; }
    volatile uint32_t* dataCommandRegister() const { return &_instance->hw->data_cmd; }
    uint dmaDreq(bool isTx) const { return i2c_get_dreq(_instance, isTx); }

    void setTarget(uint8_t address) { _instance->hw->tar = address; }
    void setDmaEnabled(bool txEnabled, bool rxEnabled);

  private:
    i2c_inst_t* _instance;
};

#endif
