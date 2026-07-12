#include "FailsafeManager.h"

void FailsafeManager::init() {
}

FailsafeDecision FailsafeManager::evaluate(const FlightData& data) const {
    uint16_t reasons = FailsafeNone;
    if (data.failsafe) {
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

    if (reasons & FailsafeRcLoss) return {true, "RC failsafe", reasons};
    if (reasons & FailsafeSensorInvalid) return {true, "Sensor unhealthy", reasons};
    if (reasons & FailsafeEstimatorInvalid) return {true, "Estimator unhealthy", reasons};
    if (reasons & FailsafeTiming) return {true, "Timing budget exceeded", reasons};
    if (reasons & FailsafeBatteryCritical) return {true, "Battery critical", reasons};
    if (reasons & FailsafeActuator) return {true, "Actuator fault", reasons};
    return {false, "OK", FailsafeNone};
}
