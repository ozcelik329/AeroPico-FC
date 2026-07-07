#ifndef COMPLEMENTARY_ESTIMATOR_H
#define COMPLEMENTARY_ESTIMATOR_H

#include "../types.h"

class ComplementaryEstimator {
  public:
    void init(float altitudeAlpha = 0.15f);
    EstimatedState update(const FlightData& flightData, float baroAltitudeM, bool baroValid);
    EstimatedState getState() const;
    void reset();

  private:
    float _altitudeAlpha = 0.15f;
    EstimatedState _state = {};
    bool _hasAltitude = false;
    float _lastAltitudeM = 0.0f;
    uint32_t _lastAltitudeTimestamp = 0;
};

#endif
