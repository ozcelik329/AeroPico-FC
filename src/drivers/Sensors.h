#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include "../config.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "pico/platform.h"
#include "IDrivers.h"

#ifdef USE_GY87
  #include <Adafruit_HMC5883_U.h>
  #include <Adafruit_BMP085_U.h>
#endif

#define MPU6050_ADDR        0x68
#define MPU6050_REG_PWR     0x6B
#define MPU6050_REG_ACCEL   0x3B
#define MPU6050_REG_GYRO    0x43
#define MPU6050_REG_GYRO_CFG  0x1B
#define MPU6050_REG_ACCEL_CFG 0x1C
#define MPU6050_REG_DLPF    0x1A
#define MPU6050_REG_TEMP    0x41  // Sıcaklık register'ı

#define MPU6050_RAW_LEN     14

#define ACCEL_SCALE  (1.0f / 4096.0f)
#define GYRO_SCALE   (1.0f / 65.5f)

// IIR Low-Pass Filter alpha değeri (0.0-1.0)
// Düşük alpha = daha fazla filtreleme, daha fazla gecikme
// Yüksek alpha = daha az filtreleme, daha az gecikme
#define IIR_ALPHA    0.15f

#include "../types.h"

class SensorManager : public IImuDriver, public IMagDriver, public IBaroDriver, public IGpsDriver {
  public:
    void init();
    void update();
    SensorBuffer getLatest();

    // Driver capability queries
    bool hasMag() const override;
    bool hasBaro() const override;

    #ifdef USE_GY87
      bool _hasMag  = false;
      bool _hasBaro = false;
    #endif

  private:
    int _dma_chan = -1;
    uint8_t _dma_buf[MPU6050_RAW_LEN];
    uint8_t _reg_addr = MPU6050_REG_ACCEL;

    // IIR filter state
    float _ax_f = 0.0f, _ay_f = 0.0f, _az_f = 0.0f;

    void _mpu_write_reg(uint8_t reg, uint8_t val);
    void _mpu_start_dma_read();
    bool _mpu_dma_ready();
    void _mpu_parse(SensorBuffer& buf);  // __not_in_flash_func kaldırıldı

    #ifdef USE_GY87
        Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
        Adafruit_BMP085_Unified  bmp = Adafruit_BMP085_Unified(10085);
    #endif

    SensorBuffer _buf[2];
    volatile uint8_t _writeIdx = 0;
    mutex_t _mutex;
};

#endif