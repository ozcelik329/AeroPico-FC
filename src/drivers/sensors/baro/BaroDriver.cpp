#include "BaroDriver.h"

bool BaroDriver::loadCalibration(const uint8_t calibrationData[22]) {
    return _backend.loadCalibration(calibrationData);
}

void BaroDriver::setRawTemperature(int32_t rawTemperature) {
    _backend.setRawTemperature(rawTemperature);
}

bool BaroDriver::applyRawPressure(int32_t rawPressure, SensorBuffer& buffer) const {
    return _backend.applyRawPressure(rawPressure, buffer);
}
