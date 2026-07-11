#ifndef BARO_VERTICAL_KALMAN_H
#define BARO_VERTICAL_KALMAN_H

#include "../types.h"

struct BaroVerticalKalmanConfig {
    float altitudeProcessNoise;
    float velocityProcessNoise;
    float baroMeasurementNoise;
    float innovationGateM;
};

class BaroVerticalKalman {
  public:
    void init(const BaroVerticalKalmanConfig& config = {0.08f, 0.35f, 2.5f, 12.0f});
    void reset();
    EstimatedState update(const EstimatorInput& input, float baroAltitudeM, bool baroValid);
    EstimatedState getState() const;
    float getLastInnovationM() const { return _lastInnovationM; }
    bool wasLastMeasurementRejected() const { return _lastMeasurementRejected; }
    float getAltitudeVariance() const { return _p00; }
    float getVelocityVariance() const { return _p11; }
    uint8_t consecutiveRejects() const { return _consecutiveRejects; }

  private:
    BaroVerticalKalmanConfig _config = {};
    EstimatedState _state = {};
    bool _initialized = false;
    uint32_t _lastTimestampUs = 0;
    float _altitudeM = 0.0f;
    float _verticalSpeedMps = 0.0f;
    float _p00 = 4.0f;
    float _p01 = 0.0f;
    float _p10 = 0.0f;
    float _p11 = 4.0f;
    float _lastInnovationM = 0.0f;
    bool _lastMeasurementRejected = false;
    uint8_t _consecutiveRejects = 0;

    float clampDt(uint32_t nowUs);
    void predict(float dt);
    bool correct(float baroAltitudeM);
    void copyAttitude(const EstimatorInput& input);
    bool stateFinite() const;
    void constrainCovariance();
    void updateEstimatorHealth(bool baroValid);
};

#endif
