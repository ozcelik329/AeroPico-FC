#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "../config.h"
#include "../filters/RunningMedian.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "pico/platform.h"
#include "IDrivers.h"

#ifdef USE_GY87
  #define HMC5883L_ADDR          0x1E
  #define HMC5883L_REG_CONFIG_A  0x00
  #define HMC5883L_REG_CONFIG_B  0x01
  #define HMC5883L_REG_MODE      0x02
  #define HMC5883L_REG_DATA_X_MSB 0x03

  #define BMP085_ADDR            0x77
  #define BMP085_REG_CALIB_START 0xAA
  #define BMP085_REG_CONTROL     0xF4
  #define BMP085_REG_RESULT      0xF6
  #define BMP085_CMD_TEMP        0x2E
  #define BMP085_CMD_PRESSURE    0x34
#endif

#define MPU6050_ADDR        0x68
#define MPU6050_REG_PWR     0x6B
#define MPU6050_REG_ACCEL   0x3B
#define MPU6050_REG_GYRO    0x43
#define MPU6050_REG_GYRO_CFG  0x1B
#define MPU6050_REG_ACCEL_CFG 0x1C
#define MPU6050_REG_DLPF    0x1A
#define MPU6050_REG_WHOAMI  0x75
#define MPU6050_WHOAMI_VAL  0x68

#define MPU6050_RAW_LEN     14

#define ACCEL_SCALE  (1.0f / 4096.0f)
#define GYRO_SCALE   (1.0f / 65.5f)

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

    int _dma_chan = -1;
    int _mpu_tx_dma_chan = -1;
    uint8_t _dma_buf[MPU6050_RAW_LEN];
    uint16_t _mpu_dma_cmd[1 + MPU6050_RAW_LEN];
    uint8_t _reg_addr = MPU6050_REG_ACCEL;

    // IIR filter state
    float _ax_f = 0.0f, _ay_f = 0.0f, _az_f = 0.0f;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _axMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _ayMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _azMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _gxMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _gyMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _gzMedian;
    uint32_t _lastSensorDebugLogMs = 0;

    // Boot kalibrasyon ofsetleri
    float _gyroBiasX = 0.0f, _gyroBiasY = 0.0f, _gyroBiasZ = 0.0f;
    float _accelBiasX = 0.0f, _accelBiasY = 0.0f, _accelBiasZ = 0.0f;
    ImuCalibration _imuCalibration = {};
    MagCalibration _magCalibration = {};
    bool _magCalCollecting = false;
    float _magMinX = 0.0f, _magMinY = 0.0f, _magMinZ = 0.0f;
    float _magMaxX = 0.0f, _magMaxY = 0.0f, _magMaxZ = 0.0f;

    bool _readWhoAmI(uint8_t& whoami);
    bool _readRawSample(int16_t& raw_ax, int16_t& raw_ay, int16_t& raw_az,
                        int16_t& raw_gx, int16_t& raw_gy, int16_t& raw_gz);

    void _mpu_write_reg(uint8_t reg, uint8_t val);
    void _mpu_start_dma_read();
    bool _mpu_dma_ready();
    void _mpu_parse(SensorBuffer& buf);  // __not_in_flash_func kaldırıldı

    #ifdef USE_GY87
        int16_t AC1 = 0;
        int16_t AC2 = 0;
        int16_t AC3 = 0;
        uint16_t AC4 = 0;
        uint16_t AC5 = 0;
        uint16_t AC6 = 0;
        int16_t B1  = 0;
        int16_t B2  = 0;
        int16_t MB  = 0;
        int16_t MC  = 0;
        int16_t MD  = 0;

        int _i2c_dma_chan = -1;
        uint8_t _hmc_dma_buf[6];
        uint8_t _bmp_dma_buf[3];
        enum BmpState { BMP_IDLE, BMP_TEMP_PENDING, BMP_TEMP_READ, BMP_PRESSURE_PENDING, BMP_PRESSURE_READ } _bmp_state = BMP_IDLE;
        uint32_t _bmp_wait_until = 0;
        int32_t _bmp_raw_temp = 0;

        bool _initMag();
        bool _initBaro();
        bool _readMagAsync();
        bool _readBaroAsync(SensorBuffer& buf);
        bool _i2c_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val);
        bool _i2c_read_regs_dma(uint8_t slave_addr, uint8_t reg, uint8_t* dest, size_t len);
        bool _i2c_dma_busy() const;
    #endif

    SensorBuffer _buf[2];
    volatile uint8_t _writeIdx = 0;
    mutex_t _mutex;
};

#endif
