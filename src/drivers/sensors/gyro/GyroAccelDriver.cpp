#include "GyroAccelDriver.h"

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
    Mpu6050Sample sample = {};
    _backend.parseRaw(raw, calibration, sample);

    buffer.gx = _gxMedian.update(sample.gx);
    buffer.gy = _gyMedian.update(sample.gy);
    buffer.gz = _gzMedian.update(sample.gz);

    float axRaw = _axMedian.update(sample.ax);
    float ayRaw = _ayMedian.update(sample.ay);
    float azRaw = _azMedian.update(sample.az);

    _axFiltered = IIR_ALPHA * axRaw + (1.0f - IIR_ALPHA) * _axFiltered;
    _ayFiltered = IIR_ALPHA * ayRaw + (1.0f - IIR_ALPHA) * _ayFiltered;
    _azFiltered = IIR_ALPHA * azRaw + (1.0f - IIR_ALPHA) * _azFiltered;

    buffer.ax = _axFiltered;
    buffer.ay = _ayFiltered;
    buffer.az = _azFiltered;
    buffer.gyroTempCoeff = sample.gyroTempCoeff;
    buffer.tempC = sample.tempC;
    buffer.timestamp = timestampUs;
    buffer.valid = true;
    buffer.health = SensorHealth::Ok;
}
