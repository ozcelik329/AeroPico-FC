#include "SensorCalibration.h"

#include <math.h>
#include "gyro/GyroAccelDriver.h"

namespace {
constexpr float DEFAULT_GYRO_TEMP_COEFF = 0.004f;
constexpr float MIN_TEMP_SPAN_C = 0.10f;

static inline float clampFloat(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}
}

void SensorCalibration::reset() {
    _sumGx = 0.0f;
    _sumGy = 0.0f;
    _sumGz = 0.0f;
    _sumAx = 0.0f;
    _sumAy = 0.0f;
    _sumAz = 0.0f;
    _sumTemp = 0.0f;
    _sumTemp2 = 0.0f;
    _sumGyroMag = 0.0f;
    _sumTempGyroMag = 0.0f;
    _minTemp = 1000.0f;
    _maxTemp = -1000.0f;
    _validSamples = 0;
}

void SensorCalibration::observeMpuRaw(const MpuRawCalibrationSample& sample) {
    const float gx = sample.gx * GyroAccelDriver::GYRO_SCALE;
    const float gy = sample.gy * GyroAccelDriver::GYRO_SCALE;
    const float gz = sample.gz * GyroAccelDriver::GYRO_SCALE;
    const float tempC = (float)sample.temp / 340.0f + 36.53f;
    const float gyroMag = sqrtf(gx * gx + gy * gy + gz * gz);

    _sumGx += gx;
    _sumGy += gy;
    _sumGz += gz;
    _sumAx += sample.ax * GyroAccelDriver::ACCEL_SCALE;
    _sumAy += sample.ay * GyroAccelDriver::ACCEL_SCALE;
    _sumAz += sample.az * GyroAccelDriver::ACCEL_SCALE;
    _sumTemp += tempC;
    _sumTemp2 += tempC * tempC;
    _sumGyroMag += gyroMag;
    _sumTempGyroMag += tempC * gyroMag;
    if (tempC < _minTemp) _minTemp = tempC;
    if (tempC > _maxTemp) _maxTemp = tempC;
    _validSamples++;
}

SensorCalibrationResult SensorCalibration::finish(uint16_t minSamples) const {
    SensorCalibrationResult result = {};
    result.validSamples = _validSamples;

    if (_validSamples < minSamples || _validSamples == 0) {
        return result;
    }

    const float samples = (float)_validSamples;
    const float denom = samples * _sumTemp2 - _sumTemp * _sumTemp;
    result.tempSpanC = _maxTemp - _minTemp;

    float gyroTempCoeff = DEFAULT_GYRO_TEMP_COEFF;
    if (result.tempSpanC >= MIN_TEMP_SPAN_C && fabsf(denom) > 1.0e-6f) {
        gyroTempCoeff = fabsf((samples * _sumTempGyroMag - _sumTemp * _sumGyroMag) / denom);
        gyroTempCoeff = clampFloat(gyroTempCoeff, 0.0f, 0.05f);
    }

    result.imu = {
        _sumGx / samples,
        _sumGy / samples,
        _sumGz / samples,
        _sumAx / samples,
        _sumAy / samples,
        (_sumAz / samples) - 1.0f,
        gyroTempCoeff,
        true
    };
    result.valid = true;
    return result;
}
