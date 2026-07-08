#ifndef EKF_LITE_ESTIMATOR_H
#define EKF_LITE_ESTIMATOR_H

#include "../types.h"

struct EkfLiteConfig {
    float altitudeProcessNoise;
    float velocityProcessNoise;
    float baroMeasurementNoise;
    float innovationGateM;
};

class EkfLiteEstimator {
  public:
    void init(const EkfLiteConfig& config = {0.08f, 0.35f, 2.5f, 12.0f});
    void reset();
    EstimatedState update(const EstimatorInput& input, float baroAltitudeM, bool baroValid);
    EstimatedState getState() const;
    float getLastInnovationM() const { return _lastInnovationM; }
    bool wasLastMeasurementRejected() const { return _lastMeasurementRejected; }

  private:
    EkfLiteConfig _config = {};
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

    float clampDt(uint32_t nowUs);
    void predict(float dt);
    bool correct(float baroAltitudeM);
    void copyAttitude(const EstimatorInput& input);
};

#endif
