#ifndef BARO_DRIVER_H
#define BARO_DRIVER_H

#include "../../../types.h"
#include "Bmp085Backend.h"

class BaroDriver {
  public:
    bool loadCalibration(const uint8_t calibrationData[22]);
    void setRawTemperature(int32_t rawTemperature);
    bool applyRawPressure(int32_t rawPressure, SensorBuffer& buffer) const;

  private:
    Bmp085Backend _backend;
};

#endif
