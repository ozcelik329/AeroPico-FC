#include "Mpu6050Backend.h"

#include <cmath>
#include <Arduino.h>

int16_t Mpu6050Backend::raw16(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

float Mpu6050Backend::effectiveGyroTempCoeff(const ImuCalibration& calibration) {
    constexpr float DEFAULT_GYRO_TEMP_COEFF = 0.004f;
    if (!calibration.valid || calibration.gyroTempCoeff <= 0.0f || !std::isfinite(calibration.gyroTempCoeff)) {
        return DEFAULT_GYRO_TEMP_COEFF;
    }
    return constrain(calibration.gyroTempCoeff, 0.0f, 0.05f);
}

bool Mpu6050Backend::parseRaw(const uint8_t raw[RAW_LEN],
                              const ImuCalibration& calibration,
                              Mpu6050Sample& sample) const {
    const int16_t rawAx = raw16(raw[0], raw[1]);
    const int16_t rawAy = raw16(raw[2], raw[3]);
    const int16_t rawAz = raw16(raw[4], raw[5]);
    const int16_t rawTemp = raw16(raw[6], raw[7]);
    const int16_t rawGx = raw16(raw[8], raw[9]);
    const int16_t rawGy = raw16(raw[10], raw[11]);
    const int16_t rawGz = raw16(raw[12], raw[13]);

    sample.ax = rawAx * ACCEL_SCALE - calibration.accelBiasX;
    sample.ay = rawAy * ACCEL_SCALE - calibration.accelBiasY;
    sample.az = rawAz * ACCEL_SCALE - calibration.accelBiasZ;
    sample.gx = rawGx * GYRO_SCALE - calibration.gyroBiasX;
    sample.gy = rawGy * GYRO_SCALE - calibration.gyroBiasY;
    sample.gz = rawGz * GYRO_SCALE - calibration.gyroBiasZ;
    sample.tempC = (float)rawTemp / 340.0f + 36.53f;
    sample.gyroTempCoeff = effectiveGyroTempCoeff(calibration);
    return true;
}
