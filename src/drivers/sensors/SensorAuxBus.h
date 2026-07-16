#ifndef SENSOR_AUX_BUS_H
#define SENSOR_AUX_BUS_H

#include <Arduino.h>
#include "board/Config.h"
#include "../../types.h"
#include "../../hal/rp2350/RP2350_I2C.h"
#include "SensorDmaBus.h"
#include "SensorDeviceProfile.h"
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
    bool hasUnsupportedMag() const { return _unsupportedMagDetected; }
    uint8_t unsupportedMagAddress() const { return _unsupportedMagAddress; }

  private:
    enum BmpState : uint8_t {
        BMP_IDLE,
        BMP_TEMP_PENDING,
        BMP_TEMP_READ,
        BMP_PRESSURE_PENDING,
        BMP_PRESSURE_READ
    };

    enum AuxReadKind : uint8_t {
        AUX_NONE,
        AUX_MAG,
        AUX_BARO_TEMP,
        AUX_BARO_PRESSURE
    };

    uint8_t _hmcDmaBuf[6] = {};
    uint8_t _bmpDmaBuf[3] = {};
    const MagDeviceProfile* _magProfile = nullptr;
    const BaroDeviceProfile* _baroProfile = nullptr;
    BmpState _bmpState = BMP_IDLE;
    AuxReadKind _auxReadKind = AUX_NONE;
    uint32_t _bmpWaitUntilUs = 0;
    bool _hasMag = false;
    bool _hasBaro = false;
    bool _magTurn = true;
    bool _unsupportedMagDetected = false;
    uint8_t _unsupportedMagAddress = 0;
    bool _auxDmaEnabled = true;
    uint8_t _auxDmaTimeoutStreak = 0;
    uint32_t _auxDmaTimeouts = 0;
    uint32_t _auxPollingRecoveries = 0;

    bool writeReg(RP2350I2C& bus, uint8_t address, uint8_t reg, uint8_t value, SensorFaultCode& faultCode);
    bool readRegsNoFault(RP2350I2C& bus, uint8_t address, uint8_t reg, uint8_t* dest, size_t len);
    bool readRegsDma(SensorDmaBus& dmaBus,
                     RP2350I2C& bus,
                     uint8_t address,
                     uint8_t reg,
                     uint8_t* dest,
                     size_t len,
                     SensorFaultCode& faultCode);
    bool startRegsDma(SensorDmaBus& dmaBus,
                      RP2350I2C& bus,
                      uint8_t address,
                      uint8_t reg,
                      uint8_t* dest,
                      size_t len,
                      AuxReadKind kind,
                      SensorFaultCode& faultCode);
    bool readRegsPolling(RP2350I2C& bus,
                         uint8_t address,
                         uint8_t reg,
                         uint8_t* dest,
                         size_t len,
                         SensorFaultCode& faultCode);
    bool processPendingRead(SensorDmaBus& dmaBus,
                            RP2350I2C& bus,
                            MagDriver& magDriver,
                            BaroDriver& baroDriver,
                            SensorBuffer& buffer,
                            SensorFaultCode& faultCode);
    bool finishReadFromBuffer(AuxReadKind kind,
                              MagDriver& magDriver,
                              BaroDriver& baroDriver,
                              SensorBuffer& buffer,
                              SensorFaultCode& faultCode);
    bool recoverTimedOutRead(RP2350I2C& bus,
                             AuxReadKind kind,
                             MagDriver& magDriver,
                             BaroDriver& baroDriver,
                             SensorBuffer& buffer,
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
