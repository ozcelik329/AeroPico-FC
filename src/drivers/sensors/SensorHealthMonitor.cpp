#include "SensorHealthMonitor.h"

SensorQuality SensorHealthMonitor::evaluateQuality(bool imuAvailable,
                                                   bool sampleValid,
                                                   uint32_t sampleTimestampUs,
                                                   uint32_t nowUs,
                                                   uint32_t staleTimeoutUs) const {
    SensorQuality quality = {};
    quality.ageUs = sampleValid ? (uint32_t)(nowUs - sampleTimestampUs) : 0;

    if (!imuAvailable) {
        quality.health = SensorHealth::Invalid;
        quality.score = 0;
        return quality;
    }

    if (!sampleValid) {
        quality.health = SensorHealth::WarmingUp;
        quality.score = 25;
        return quality;
    }

    if (quality.ageUs > staleTimeoutUs) {
        quality.health = SensorHealth::Stale;
        quality.score = 0;
        return quality;
    }

    quality.health = SensorHealth::Ok;
    quality.score = (uint8_t)(100u - ((quality.ageUs * 50u) / staleTimeoutUs));
    if (quality.score > 100u) quality.score = 100u;
    if (quality.score < 50u) quality.score = 50u;
    return quality;
}

SensorHealth SensorHealthMonitor::evaluate(bool imuAvailable,
                                           bool sampleValid,
                                           uint32_t sampleTimestampUs,
                                           uint32_t nowUs,
                                           uint32_t staleTimeoutUs) const {
    return evaluateQuality(
        imuAvailable,
        sampleValid,
        sampleTimestampUs,
        nowUs,
        staleTimeoutUs
    ).health;
}
