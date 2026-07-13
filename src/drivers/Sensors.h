#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "../config.h"
#include "pico/platform.h"
#include "IDrivers.h"
#include "sensors/SensorHealthMonitor.h"
#include "sensors/SensorCalibration.h"
#include "sensors/SensorBackendRegistry.h"
#include "sensors/SensorDmaBus.h"
#include "sensors/SensorAuxBus.h"
#include "sensors/baro/BaroDriver.h"
#include "sensors/gyro/GyroAccelDriver.h"
#include "sensors/mag/MagDriver.h"
#include "../hal/HAL_I2C.h"
#include "../hal/rp2350/RP2350_I2C.h"

#define MPU6050_RAW_LEN     SensorDmaBus::MPU_RAW_LEN

// IIR Low-Pass Filter alpha değeri (0.0-1.0)
// Düşük alpha = daha fazla filtreleme, daha fazla gecikme
// Yüksek alpha = daha az filtreleme, daha az gecikme
#define MPU6050_REG_TEMP    0x41  // Sıcaklık register'ı

#define IIR_ALPHA    0.15f
#define BOOT_CAL_SAMPLES    256

#include "../types.h"

class SensorManager : public IImuDriver, public IMagDriver, public IBaroDriver, public IGpsDriver {
  public:
    void init();
    void setI2CBus(IHALI2C* bus);
    void setI2CBus(RP2350I2C* bus);
    void update();
    SensorBuffer getLatest();

    bool isImuAvailable() const override;
    bool isDmaOk() const override;
    SensorCapabilityStatus capabilities() const;
    SensorFaultCode getFaultCode() const;
    const char* getFaultText() const;
    uint8_t getLastWhoAmI() const { return _lastWhoAmI; }
    bool runBootCalibration() override;
    ImuCalibration getImuCalibration() const;
    void setImuCalibration(const ImuCalibration& calibration);

    // Driver capability queries
    bool hasMag() const override;
    bool hasBaro() const override;
    void beginMagCalibration();
    bool observeMagCalibrationSample(float mx, float my, float mz);
    MagCalibration finishMagCalibration();
    MagCalibration getMagCalibration() const;
    void setMagCalibration(const MagCalibration& calibration);

    #ifdef USE_GY87
      bool _hasMag  = false;
      bool _hasBaro = false;
    #endif

  private:
    bool _imuAvailable = false;
    uint8_t _lastWhoAmI = 0;
    SensorFaultCode _faultCode = SensorFaultCode::None;

    const ImuDeviceProfile* _imuProfile = &SensorBackendRegistry::mpu6050();
    uint8_t _reg_addr = SensorBackendRegistry::mpu6050().accelReg;

    SensorDmaBus _dmaBus;
#ifdef USE_GY87
    SensorAuxBus _auxBus;
#endif
    GyroAccelDriver _gyroAccelDriver;
    MagDriver _magDriver;
    BaroDriver _baroDriver;
    SensorHealthMonitor _healthMonitor;
    SensorCalibration _calibration;
    IHALI2C* _i2cBus = nullptr;
    RP2350I2C* _rp2350Bus = nullptr;
    bool _dmaFastPath = false;

    // Boot kalibrasyon ofsetleri
    float _gyroBiasX = 0.0f, _gyroBiasY = 0.0f, _gyroBiasZ = 0.0f;
    float _accelBiasX = 0.0f, _accelBiasY = 0.0f, _accelBiasZ = 0.0f;
    ImuCalibration _imuCalibration = {};

    bool _readWhoAmI(uint8_t& whoami);
    bool _readRawSample(int16_t& raw_ax, int16_t& raw_ay, int16_t& raw_az,
                        int16_t& raw_gx, int16_t& raw_gy, int16_t& raw_gz,
                        int16_t* raw_temp = nullptr);

    void _mpu_write_reg(uint8_t reg, uint8_t val);
    void _mpu_start_dma_read();
    bool _mpu_dma_ready();
    bool _readRawFrame(uint8_t raw[GyroAccelDriver::RAW_LEN]);
    void _setFault(SensorFaultCode code);
    IHALI2C& _bus();
    RP2350I2C* _rpBus();

    SensorBuffer _buf[2];
    volatile uint8_t _writeIdx = 0;
    mutex_t _mutex;
};

#endif
