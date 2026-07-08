#ifndef SENSOR_HEALTH_MONITOR_H
#define SENSOR_HEALTH_MONITOR_H

#include "../../types.h"

class SensorHealthMonitor {
  public:
    SensorQuality evaluateQuality(bool imuAvailable,
                                  bool sampleValid,
                                  uint32_t sampleTimestampUs,
                                  uint32_t nowUs,
                                  uint32_t staleTimeoutUs) const;

    SensorHealth evaluate(bool imuAvailable,
                          bool sampleValid,
                          uint32_t sampleTimestampUs,
                          uint32_t nowUs,
                          uint32_t staleTimeoutUs) const;
};

#endif
