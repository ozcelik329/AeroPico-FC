#include "GyroAccelDriver.h"

static int16_t gyroToInt16(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

static float effectiveGyroTempCoeff(const ImuCalibration& calibration) {
    constexpr float DEFAULT_GYRO_TEMP_COEFF = 0.004f;
    if (!calibration.valid || calibration.gyroTempCoeff <= 0.0f || !isfinite(calibration.gyroTempCoeff)) {
        return DEFAULT_GYRO_TEMP_COEFF;
    }
    return constrain(calibration.gyroTempCoeff, 0.0f, 0.05f);
}

void GyroAccelDriver::resetFilters() {
    _axFiltered = 0.0f;
    _ayFiltered = 0.0f;
    _azFiltered = 0.0f;
    _axMedian.reset();
    _ayMedian.reset();
    _azMedian.reset();
    _gxMedian.reset();
    _gyMedian.reset();
    _gzMedian.reset();
}

void GyroAccelDriver::parseRawSample(const uint8_t raw[RAW_LEN],
                                     const ImuCalibration& calibration,
                                     SensorBuffer& buffer,
                                     uint32_t timestampUs) {
    int16_t rawAx = gyroToInt16(raw[0], raw[1]);
    int16_t rawAy = gyroToInt16(raw[2], raw[3]);
    int16_t rawAz = gyroToInt16(raw[4], raw[5]);
    int16_t rawTemp = gyroToInt16(raw[6], raw[7]);
    int16_t rawGx = gyroToInt16(raw[8], raw[9]);
    int16_t rawGy = gyroToInt16(raw[10], raw[11]);
    int16_t rawGz = gyroToInt16(raw[12], raw[13]);

    float gxRaw = rawGx * GYRO_SCALE - calibration.gyroBiasX;
    float gyRaw = rawGy * GYRO_SCALE - calibration.gyroBiasY;
    float gzRaw = rawGz * GYRO_SCALE - calibration.gyroBiasZ;

    buffer.gx = _gxMedian.update(gxRaw);
    buffer.gy = _gyMedian.update(gyRaw);
    buffer.gz = _gzMedian.update(gzRaw);

    float axRaw = rawAx * ACCEL_SCALE - calibration.accelBiasX;
    float ayRaw = rawAy * ACCEL_SCALE - calibration.accelBiasY;
    float azRaw = rawAz * ACCEL_SCALE - calibration.accelBiasZ;

    axRaw = _axMedian.update(axRaw);
    ayRaw = _ayMedian.update(ayRaw);
    azRaw = _azMedian.update(azRaw);

    _axFiltered = IIR_ALPHA * axRaw + (1.0f - IIR_ALPHA) * _axFiltered;
    _ayFiltered = IIR_ALPHA * ayRaw + (1.0f - IIR_ALPHA) * _ayFiltered;
    _azFiltered = IIR_ALPHA * azRaw + (1.0f - IIR_ALPHA) * _azFiltered;

    buffer.ax = _axFiltered;
    buffer.ay = _ayFiltered;
    buffer.az = _azFiltered;
    buffer.gyroTempCoeff = effectiveGyroTempCoeff(calibration);
    buffer.tempC = (float)rawTemp / 340.0f + 36.53f;
    buffer.timestamp = timestampUs;
    buffer.valid = true;
    buffer.health = SensorHealth::Ok;
}
