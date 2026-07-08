#ifndef BARO_ALTITUDE_ESTIMATOR_H
#define BARO_ALTITUDE_ESTIMATOR_H

#include <Arduino.h>

class BaroAltitudeEstimator {
  public:
    void init(float metersPerHpa = 8.43f);
    void reset();
    void setHomePressure(float pressureHpa);
    bool update(float pressureHpa, bool pressureValid, float& altitudeM);
    bool hasHome() const { return _hasHome; }
    float getHomePressureHpa() const { return _homePressureHpa; }

  private:
    float _metersPerHpa = 8.43f;
    float _homePressureHpa = 1013.25f;
    bool _hasHome = false;
};

#endif
