#ifndef SENSOR_PREFLIGHT_EVALUATOR_H
#define SENSOR_PREFLIGHT_EVALUATOR_H

#include <Arduino.h>
#include "../types.h"

enum class SensorPreflightReason : uint8_t {
    Ok = 0,
    ImuUnavailable,
    HealthNotOk,
    QualityLow
};

struct SensorPreflightStatus {
    bool passed;
    SensorPreflightReason reason;
    SensorHealth health;
    uint8_t qualityScore;
    uint32_t sampleAgeUs;
};

class SensorPreflightEvaluator {
  public:
    static SensorPreflightStatus evaluate(bool imuAvailable,
                                          const SensorBuffer& sample,
                                          uint8_t minQuality);
    static void formatReason(const SensorPreflightStatus& status,
                             char* destination,
                             size_t destinationSize);
};

#endif
