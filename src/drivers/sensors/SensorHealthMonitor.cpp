#include "SensorHealthMonitor.h"

SensorHealth SensorHealthMonitor::evaluate(bool imuAvailable,
                                           bool sampleValid,
                                           uint32_t sampleTimestampUs,
                                           uint32_t nowUs,
                                           uint32_t staleTimeoutUs) const {
    if (!imuAvailable) {
        return SensorHealth::Invalid;
    }

    if (!sampleValid) {
        return SensorHealth::WarmingUp;
    }

    if ((uint32_t)(nowUs - sampleTimestampUs) > staleTimeoutUs) {
        return SensorHealth::Stale;
    }

    return SensorHealth::Ok;
}
