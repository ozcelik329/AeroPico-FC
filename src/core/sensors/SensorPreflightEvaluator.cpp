#include "SensorPreflightEvaluator.h"

#include <stdio.h>

static const char* sensorHealthReasonToken(SensorHealth health) {
    switch (health) {
        case SensorHealth::Ok: return "OK";
        case SensorHealth::WarmingUp: return "WARMUP";
        case SensorHealth::Stale: return "STALE";
        case SensorHealth::Timeout: return "TIMEOUT";
        case SensorHealth::Invalid:
        default: return "INVALID";
    }
}

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
                     "IMU_%s age=%uus",
                     sensorHealthReasonToken(status.health),
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
