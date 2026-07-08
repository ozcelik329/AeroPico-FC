#ifndef MAG_DRIVER_H
#define MAG_DRIVER_H

#include "../../../types.h"

class MagDriver {
  public:
    void beginCalibration();
    bool observeCalibrationSample(float mx, float my, float mz);
    MagCalibration finishCalibration();
    MagCalibration getCalibration() const;
    void setCalibration(const MagCalibration& calibration);
    void applySample(int16_t rawX, int16_t rawY, int16_t rawZ, SensorBuffer& buffer);

  private:
    MagCalibration _calibration = {};
    bool _collecting = false;
    float _minX = 0.0f;
    float _minY = 0.0f;
    float _minZ = 0.0f;
    float _maxX = 0.0f;
    float _maxY = 0.0f;
    float _maxZ = 0.0f;
};

#endif
