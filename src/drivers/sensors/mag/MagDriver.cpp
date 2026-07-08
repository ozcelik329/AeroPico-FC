#include "MagDriver.h"
#include <Arduino.h>

static float minFloat(float a, float b) {
    return a < b ? a : b;
}

static float maxFloat(float a, float b) {
    return a > b ? a : b;
}

void MagDriver::beginCalibration() {
    _collecting = true;
    _minX = _minY = _minZ = 1000000.0f;
    _maxX = _maxY = _maxZ = -1000000.0f;
}

bool MagDriver::observeCalibrationSample(float mx, float my, float mz) {
    if (!_collecting) {
        return false;
    }

    _minX = minFloat(_minX, mx);
    _minY = minFloat(_minY, my);
    _minZ = minFloat(_minZ, mz);
    _maxX = maxFloat(_maxX, mx);
    _maxY = maxFloat(_maxY, my);
    _maxZ = maxFloat(_maxZ, mz);
    return true;
}

MagCalibration MagDriver::finishCalibration() {
    _collecting = false;
    _calibration.hardIronX = (_minX + _maxX) * 0.5f;
    _calibration.hardIronY = (_minY + _maxY) * 0.5f;
    _calibration.hardIronZ = (_minZ + _maxZ) * 0.5f;
    _calibration.valid = true;
    return _calibration;
}

MagCalibration MagDriver::getCalibration() const {
    return _calibration;
}

void MagDriver::setCalibration(const MagCalibration& calibration) {
    if (!calibration.valid) {
        return;
    }
    _calibration = calibration;
}

void MagDriver::applySample(int16_t rawX, int16_t rawY, int16_t rawZ, SensorBuffer& buffer) {
    float mxScaled = rawX * 0.92f;
    float myScaled = rawY * 0.92f;
    float mzScaled = rawZ * 0.92f;

    observeCalibrationSample(mxScaled, myScaled, mzScaled);
    buffer.mx = mxScaled - (_calibration.valid ? _calibration.hardIronX : 0.0f);
    buffer.my = myScaled - (_calibration.valid ? _calibration.hardIronY : 0.0f);
    buffer.mz = mzScaled - (_calibration.valid ? _calibration.hardIronZ : 0.0f);
}
