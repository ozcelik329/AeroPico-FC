#ifndef SENSOR_PIPELINE_H
#define SENSOR_PIPELINE_H

#include <Arduino.h>
#include "../drivers/IDrivers.h"
#include "../types.h"
#include "SensorFusion.h"

class SensorPipeline {
  public:
    void init(IImuDriver* imuDriver, float fusionBeta = 0.08f);
    VehicleState update();
    VehicleState getState() const;

  private:
    IImuDriver* _imu = nullptr;
    SensorFusion _fusion;
    VehicleState _state = {};

    void updateFusion(const SensorBuffer& buffer);
};

#endif
