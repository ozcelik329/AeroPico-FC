#ifndef SENSOR_CALIBRATION_H
#define SENSOR_CALIBRATION_H

#include <stdint.h>
#include "../../types.h"

struct MpuRawCalibrationSample {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t temp;
};

struct SensorCalibrationResult {
    ImuCalibration imu;
    uint16_t validSamples;
    float tempSpanC;
    bool valid;
};

class SensorCalibration {
  public:
    void reset();
    void observeMpuRaw(const MpuRawCalibrationSample& sample);
    SensorCalibrationResult finish(uint16_t minSamples) const;

  private:
    float _sumGx = 0.0f;
    float _sumGy = 0.0f;
    float _sumGz = 0.0f;
    float _sumAx = 0.0f;
    float _sumAy = 0.0f;
    float _sumAz = 0.0f;
    float _sumTemp = 0.0f;
    float _sumTemp2 = 0.0f;
    float _sumGyroMag = 0.0f;
    float _sumTempGyroMag = 0.0f;
    float _minTemp = 1000.0f;
    float _maxTemp = -1000.0f;
    uint16_t _validSamples = 0;
};

#endif
