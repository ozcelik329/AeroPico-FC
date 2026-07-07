#include "FailsafeManager.h"

void FailsafeManager::init() {
}

FailsafeDecision FailsafeManager::evaluate(const FlightData& data) const {
    if (data.failsafe) {
        return {true, "RC failsafe"};
    }

    if (data.sensorHealth == SensorHealth::Invalid ||
        data.sensorHealth == SensorHealth::Stale ||
        data.sensorHealth == SensorHealth::Timeout) {
        return {true, "Sensor unhealthy"};
    }

    return {false, "OK"};
}
