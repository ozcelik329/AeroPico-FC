#ifndef MPU6050_BACKEND_H
#define MPU6050_BACKEND_H

#include "../../../config.h"
#include "../../../types.h"

struct Mpu6050Sample {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float tempC;
    float gyroTempCoeff;
};

class Mpu6050Backend {
  public:
    static constexpr float ACCEL_SCALE = 1.0f / 4096.0f;
    static constexpr float GYRO_SCALE = 1.0f / 65.5f;
    static constexpr uint8_t RAW_LEN = 14;

    bool parseRaw(const uint8_t raw[RAW_LEN],
                  const ImuCalibration& calibration,
                  Mpu6050Sample& sample) const;

    static int16_t raw16(uint8_t hi, uint8_t lo);
    static float effectiveGyroTempCoeff(const ImuCalibration& calibration);
};

#endif
