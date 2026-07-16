#ifndef SENSOR_DMA_BUS_H
#define SENSOR_DMA_BUS_H

#include <Arduino.h>
#include "../../hal/rp2350/RP2350_I2C.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"

class SensorDmaBus {
  public:
    static constexpr size_t MPU_RAW_LEN = 14;
    static constexpr size_t AUX_MAX_LEN = 22;

    bool configureMpu(RP2350I2C& bus);
    bool configureAuxRx(RP2350I2C& bus);

    bool hasMpuChannels() const { return _mpuRxChan >= 0 && _mpuTxChan >= 0; }
    bool hasAuxChannel() const { return _auxRxChan >= 0 && _auxTxChan >= 0; }

    bool startMpuRead(RP2350I2C& bus, uint8_t address, uint8_t reg, uint32_t nowUs);
    bool isMpuReady() const;
    bool mpuTimedOut(uint32_t nowUs, uint32_t timeoutUs) const;
    void finishMpu();
    void abortMpu();

    const uint8_t* mpuBuffer() const { return _mpuRx; }

    bool readAuxRegistersDma(RP2350I2C& bus,
                             uint8_t address,
                             uint8_t reg,
                             uint8_t* dest,
                             size_t len,
                             uint32_t timeoutUs,
                             uint32_t (*nowUs)());
    bool startAuxRead(RP2350I2C& bus,
                      uint8_t address,
                      uint8_t reg,
                      uint8_t* dest,
                      size_t len,
                      uint32_t nowUs);
    bool isAuxReady() const;
    bool auxTimedOut(uint32_t nowUs, uint32_t timeoutUs) const;
    void finishAux();
    void abortAux();

  private:
    static constexpr uint16_t READ_CMD = I2C_IC_DATA_CMD_CMD_BITS;
    static constexpr uint16_t STOP_CMD = I2C_IC_DATA_CMD_STOP_BITS;

    int _mpuRxChan = -1;
    int _mpuTxChan = -1;
    int _auxRxChan = -1;
    int _auxTxChan = -1;
    uint8_t _mpuRx[MPU_RAW_LEN] = {};
    uint16_t _mpuCmd[1 + MPU_RAW_LEN] = {};
    uint16_t _auxCmd[AUX_MAX_LEN] = {};
    uint32_t _mpuStartUs = 0;
    uint32_t _auxStartUs = 0;
    bool _mpuActive = false;
    bool _auxActive = false;

    void prepareMpuCommands(uint8_t reg);
    bool prepareAuxCommands(size_t len);
};

#endif
