#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "../config.h"
#include "pico/platform.h"
#include "IDrivers.h"
#include "sensors/SensorHealthMonitor.h"
#include "sensors/SensorDmaBus.h"
#include "sensors/SensorAuxBus.h"
#include "sensors/baro/BaroDriver.h"
#include "sensors/gyro/GyroAccelDriver.h"
#include "sensors/mag/MagDriver.h"

#define MPU6050_ADDR        0x68
#define MPU6050_REG_PWR     0x6B
#define MPU6050_REG_ACCEL   0x3B
#define MPU6050_REG_GYRO    0x43
#define MPU6050_REG_GYRO_CFG  0x1B
#define MPU6050_REG_ACCEL_CFG 0x1C
#define MPU6050_REG_DLPF    0x1A
#define MPU6050_REG_WHOAMI  0x75
#define MPU6050_WHOAMI_VAL  0x68

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
    void update();
    SensorBuffer getLatest();

    bool isImuAvailable() const override;
    bool isDmaOk() const override;
    SensorFaultCode getFaultCode() const;
    const char* getFaultText() const;
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
    SensorFaultCode _faultCode = SensorFaultCode::None;

    uint8_t _reg_addr = MPU6050_REG_ACCEL;

    SensorDmaBus _dmaBus;
#ifdef USE_GY87
    SensorAuxBus _auxBus;
#endif
    GyroAccelDriver _gyroAccelDriver;
    MagDriver _magDriver;
    BaroDriver _baroDriver;
    SensorHealthMonitor _healthMonitor;

    // Boot kalibrasyon ofsetleri
    float _gyroBiasX = 0.0f, _gyroBiasY = 0.0f, _gyroBiasZ = 0.0f;
    float _accelBiasX = 0.0f, _accelBiasY = 0.0f, _accelBiasZ = 0.0f;
    ImuCalibration _imuCalibration = {};

    bool _readWhoAmI(uint8_t& whoami);
    bool _readRawSample(int16_t& raw_ax, int16_t& raw_ay, int16_t& raw_az,
                        int16_t& raw_gx, int16_t& raw_gy, int16_t& raw_gz);

    void _mpu_write_reg(uint8_t reg, uint8_t val);
    void _mpu_start_dma_read();
    bool _mpu_dma_ready();
    void _setFault(SensorFaultCode code);

    SensorBuffer _buf[2];
    volatile uint8_t _writeIdx = 0;
    mutex_t _mutex;
};

#endif
