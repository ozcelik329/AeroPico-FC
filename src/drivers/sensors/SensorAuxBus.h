#ifndef SENSOR_AUX_BUS_H
#define SENSOR_AUX_BUS_H

#include <Arduino.h>
#include "../../config.h"
#include "../../types.h"
#include "../../hal/rp2350/RP2350_I2C.h"
#include "SensorDmaBus.h"
#include "baro/BaroDriver.h"
#include "mag/MagDriver.h"

#ifdef USE_GY87

class SensorAuxBus {
  public:
    bool configure(SensorDmaBus& dmaBus,
                   RP2350I2C& bus,
                   BaroDriver& baroDriver,
                   bool& hasMag,
                   bool& hasBaro,
                   SensorFaultCode& faultCode);
    void update(SensorDmaBus& dmaBus,
                RP2350I2C& bus,
                MagDriver& magDriver,
                BaroDriver& baroDriver,
                SensorBuffer& buffer,
                SensorFaultCode& faultCode);

  private:
    static constexpr uint8_t HMC5883L_ADDR = 0x1E;
    static constexpr uint8_t HMC5883L_REG_CONFIG_A = 0x00;
    static constexpr uint8_t HMC5883L_REG_CONFIG_B = 0x01;
    static constexpr uint8_t HMC5883L_REG_MODE = 0x02;
    static constexpr uint8_t HMC5883L_REG_DATA_X_MSB = 0x03;

    static constexpr uint8_t BMP085_ADDR = 0x77;
    static constexpr uint8_t BMP085_REG_CALIB_START = 0xAA;
    static constexpr uint8_t BMP085_REG_CONTROL = 0xF4;
    static constexpr uint8_t BMP085_REG_RESULT = 0xF6;
    static constexpr uint8_t BMP085_CMD_TEMP = 0x2E;
    static constexpr uint8_t BMP085_CMD_PRESSURE = 0x34;

    enum BmpState : uint8_t {
        BMP_IDLE,
        BMP_TEMP_PENDING,
        BMP_TEMP_READ,
        BMP_PRESSURE_PENDING,
        BMP_PRESSURE_READ
    };

    uint8_t _hmcDmaBuf[6] = {};
    uint8_t _bmpDmaBuf[3] = {};
    BmpState _bmpState = BMP_IDLE;
    uint32_t _bmpWaitUntilUs = 0;
    bool _hasMag = false;
    bool _hasBaro = false;

    bool writeReg(RP2350I2C& bus, uint8_t address, uint8_t reg, uint8_t value, SensorFaultCode& faultCode);
    bool readRegsDma(SensorDmaBus& dmaBus,
                     RP2350I2C& bus,
                     uint8_t address,
                     uint8_t reg,
                     uint8_t* dest,
                     size_t len,
                     SensorFaultCode& faultCode);
    bool initMag(RP2350I2C& bus, SensorFaultCode& faultCode);
    bool initBaro(SensorDmaBus& dmaBus, RP2350I2C& bus, BaroDriver& baroDriver, SensorFaultCode& faultCode);
    void readMag(SensorDmaBus& dmaBus,
                 RP2350I2C& bus,
                 MagDriver& magDriver,
                 SensorBuffer& buffer,
                 SensorFaultCode& faultCode);
    bool readBaro(SensorDmaBus& dmaBus,
                  RP2350I2C& bus,
                  BaroDriver& baroDriver,
                  SensorBuffer& buffer,
                  SensorFaultCode& faultCode);
};

#endif
#endif
