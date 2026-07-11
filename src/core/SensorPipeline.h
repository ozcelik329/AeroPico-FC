#ifndef SENSOR_PIPELINE_H
#define SENSOR_PIPELINE_H

#include <Arduino.h>
#include "../drivers/IDrivers.h"
#include "../types.h"
#include "../estimators/BaroAltitudeEstimator.h"
#include "../estimators/BaroVerticalKalman.h"
#include "../estimators/ComplementaryEstimator.h"
#include "SensorFusion.h"

class SensorPipeline {
  public:
    void init(IImuDriver* imuDriver, float fusionBeta = 0.08f);
    VehicleState update();
    VehicleState getState() const;

  private:
    IImuDriver* _imu = nullptr;
    SensorFusion _fusion;
    BaroAltitudeEstimator _baroAltitude;
    BaroVerticalKalman _verticalKalman;
    ComplementaryEstimator _fallbackEstimator;
    VehicleState _state = {};
    float _lastVerticalAccelMps2 = 0.0f;

    void updateFusion(const SensorBuffer& buffer);
    EstimatorInput buildEstimatorInput() const;
};

#endif
