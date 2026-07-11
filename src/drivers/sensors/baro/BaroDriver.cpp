#include "BaroDriver.h"

static int16_t baroToInt16(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

static uint16_t baroToUint16(uint8_t hi, uint8_t lo) {
    return (uint16_t)((hi << 8) | lo);
}

bool BaroDriver::loadCalibration(const uint8_t calibrationData[22]) {
    _ac1 = baroToInt16(calibrationData[0], calibrationData[1]);
    _ac2 = baroToInt16(calibrationData[2], calibrationData[3]);
    _ac3 = baroToInt16(calibrationData[4], calibrationData[5]);
    _ac4 = baroToUint16(calibrationData[6], calibrationData[7]);
    _ac5 = baroToUint16(calibrationData[8], calibrationData[9]);
    _ac6 = baroToUint16(calibrationData[10], calibrationData[11]);
    _b1 = baroToInt16(calibrationData[12], calibrationData[13]);
    _b2 = baroToInt16(calibrationData[14], calibrationData[15]);
    _mb = baroToInt16(calibrationData[16], calibrationData[17]);
    _mc = baroToInt16(calibrationData[18], calibrationData[19]);
    _md = baroToInt16(calibrationData[20], calibrationData[21]);
    _calibrated = true;
    return true;
}

void BaroDriver::setRawTemperature(int32_t rawTemperature) {
    _rawTemperature = rawTemperature;
}

bool BaroDriver::applyRawPressure(int32_t rawPressure, SensorBuffer& buffer) const {
    if (!_calibrated) {
        return false;
    }

    int32_t x1 = (_rawTemperature - _ac6) * _ac5 >> 15;
    const int32_t temperatureDenominator = x1 + _md;
    if (temperatureDenominator == 0) {
        return false;
    }
    int32_t x2 = ((int32_t)_mc << 11) / temperatureDenominator;
    int32_t b5 = x1 + x2;
    int32_t t = (b5 + 8) >> 4;
    int32_t b6 = b5 - 4000;
    int32_t x1p = (_b2 * (b6 * b6 >> 12)) >> 11;
    int32_t x2p = (_ac2 * b6) >> 11;
    int32_t x3 = x1p + x2p;
    int32_t b3 = (((int32_t)_ac1 * 4 + x3) + 2) >> 2;
    int32_t x1pp = (_ac3 * b6) >> 13;
    int32_t x2pp = (_b1 * ((b6 * b6) >> 12)) >> 16;
    int32_t x3p = ((x1pp + x2pp) + 2) >> 2;
    int32_t b4 = (_ac4 * (uint32_t)(x1pp + x3p + 32768)) >> 15;
    if (b4 == 0) {
        return false;
    }
    int32_t b7 = ((uint32_t)rawPressure - b3) * (50000 >> 3);
    int32_t p = (b7 < 0) ? (b7 * 2) / b4 : (b7 / b4) * 2;
    int32_t x1ppp = (p >> 8) * (p >> 8);
    int32_t x1pppp = (x1ppp * 3038) >> 16;
    int32_t x2pppp = (-7357 * p) >> 16;
    int32_t pressure = p + ((x1pppp + x2pppp + 3791) >> 4);

    buffer.pressureHpa = pressure * 0.01f;
    buffer.tempC = t / 10.0f;
    buffer.baroValid = true;
    return true;
}
