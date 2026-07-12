#include "SensorPreflightEvaluator.h"

#include <stdio.h>

SensorPreflightStatus SensorPreflightEvaluator::evaluate(bool imuAvailable,
                                                         const SensorBuffer& sample,
                                                         uint8_t minQuality) {
    SensorPreflightStatus status = {
        false,
        SensorPreflightReason::ImuUnavailable,
        sample.health,
        sample.qualityScore,
        sample.sampleAgeUs
    };

    if (!imuAvailable) {
        return status;
    }

    if (sample.health != SensorHealth::Ok || !sample.valid) {
        status.reason = SensorPreflightReason::HealthNotOk;
        return status;
    }

    if (sample.qualityScore < minQuality) {
        status.reason = SensorPreflightReason::QualityLow;
        return status;
    }

    status.passed = true;
    status.reason = SensorPreflightReason::Ok;
    return status;
}

void SensorPreflightEvaluator::formatReason(const SensorPreflightStatus& status,
                                            char* destination,
                                            size_t destinationSize) {
    if (!destination || destinationSize == 0) {
        return;
    }

    switch (status.reason) {
        case SensorPreflightReason::Ok:
            snprintf(destination,
                     destinationSize,
                     "Sensor OK q=%u age=%uus",
                     status.qualityScore,
                     status.sampleAgeUs);
            break;
        case SensorPreflightReason::ImuUnavailable:
            snprintf(destination, destinationSize, "IMU not available");
            break;
        case SensorPreflightReason::HealthNotOk:
            snprintf(destination,
                     destinationSize,
                     "Sensor health %u age=%uus",
                     (unsigned)status.health,
                     status.sampleAgeUs);
            break;
        case SensorPreflightReason::QualityLow:
            snprintf(destination,
                     destinationSize,
                     "Sensor quality low q=%u age=%uus",
                     status.qualityScore,
                     status.sampleAgeUs);
            break;
    }
}
