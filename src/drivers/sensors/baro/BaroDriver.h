#ifndef BARO_DRIVER_H
#define BARO_DRIVER_H

#include "../../../types.h"

class BaroDriver {
  public:
    bool loadCalibration(const uint8_t calibrationData[22]);
    void setRawTemperature(int32_t rawTemperature);
    bool applyRawPressure(int32_t rawPressure, SensorBuffer& buffer) const;

  private:
    int16_t _ac1 = 0;
    int16_t _ac2 = 0;
    int16_t _ac3 = 0;
    uint16_t _ac4 = 0;
    uint16_t _ac5 = 0;
    uint16_t _ac6 = 0;
    int16_t _b1 = 0;
    int16_t _b2 = 0;
    int16_t _mb = 0;
    int16_t _mc = 0;
    int16_t _md = 0;
    int32_t _rawTemperature = 0;
    bool _calibrated = false;
};

#endif
