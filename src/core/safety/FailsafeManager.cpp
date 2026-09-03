#include "FailsafeManager.h"

void FailsafeManager::init() {
}

FailsafeDecision FailsafeManager::evaluate(const FlightData& data,
                                           const FailsafePolicy& policy) const {
    uint16_t reasons = FailsafeNone;
    if (policy.rcRequired && data.failsafe) {
        reasons |= FailsafeRcLoss;
    }

    if (data.sensorHealth == SensorHealth::Invalid ||
        data.sensorHealth == SensorHealth::Stale ||
        data.sensorHealth == SensorHealth::Timeout) {
        reasons |= FailsafeSensorInvalid;
    }

    if (!data.estimatorValid ||
        data.estimatorHealth == SensorHealth::Invalid ||
        data.estimatorHealth == SensorHealth::Stale ||
        data.estimatorHealth == SensorHealth::Timeout) {
        reasons |= FailsafeEstimatorInvalid;
    }

    if (data.timingExceeded) {
        reasons |= FailsafeTiming;
    }

    if (data.batteryCritical) {
        reasons |= FailsafeBatteryCritical;
    }

    if (data.actuatorFault) {
        reasons |= FailsafeActuator;
    }

    const uint16_t effectiveReasons = reasons & (uint16_t)~policy.bypassMask;
    return {
        effectiveReasons != FailsafeNone,
        reasonToken(effectiveReasons),
        effectiveReasons,
        reasons,
        data.timestamp
    };
}

const char* FailsafeManager::reasonToken(uint16_t reasons) {
    if (reasons & FailsafeRcLoss) return "RC_LOSS";
    if (reasons & FailsafeSensorInvalid) return "SENSOR_INVALID";
    if (reasons & FailsafeEstimatorInvalid) return "ESTIMATOR_INVALID";
    if (reasons & FailsafeTiming) return "TIMING";
    if (reasons & FailsafeBatteryCritical) return "BATTERY_CRITICAL";
    if (reasons & FailsafeActuator) return "ACTUATOR_FAULT";
    return "NONE";
}
