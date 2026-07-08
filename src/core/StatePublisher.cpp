#include "StatePublisher.h"

FlightData StatePublisher::buildFlightData(const VehicleState& vehicle,
                                           const RcInputState& rc,
                                           const FailsafeDecision& failsafe) const {
    FlightData data;
    data.gyroX = vehicle.gyroX;
    data.gyroY = vehicle.gyroY;
    data.gyroZ = vehicle.gyroZ;
    data.altitudeM = vehicle.altitudeM;
    data.verticalSpeedMps = vehicle.verticalSpeedMps;
    data.roll = vehicle.rollDeg;
    data.pitch = vehicle.pitchDeg;
    data.yaw = vehicle.yawDeg;
    data.sensorHealth = vehicle.sensorHealth;
    data.estimatorHealth = vehicle.estimatorHealth;
    data.sensorQualityScore = vehicle.sensorQualityScore;
    data.sensorAgeUs = vehicle.sensorAgeUs;
    data.timestamp = vehicle.timestampUs;
    data.aileron = rc.aileron;
    data.elevator = rc.elevator;
    data.throttle = rc.throttle;
    data.rudder = rc.rudder;
    data.failsafe = failsafe.active;
    data.estimatorValid = vehicle.estimatorValid;
    return data;
}

SensorState StatePublisher::buildSensorState(const VehicleState& vehicle) const {
    SensorState state;
    state.rollDeg = vehicle.rollDeg;
    state.pitchDeg = vehicle.pitchDeg;
    state.yawDeg = vehicle.yawDeg;
    state.gyroX = vehicle.gyroX;
    state.gyroY = vehicle.gyroY;
    state.gyroZ = vehicle.gyroZ;
    state.altitudeM = vehicle.altitudeM;
    state.verticalSpeedMps = vehicle.verticalSpeedMps;
    state.health = vehicle.sensorHealth;
    state.estimatorHealth = vehicle.estimatorHealth;
    state.sensorQualityScore = vehicle.sensorQualityScore;
    state.sensorAgeUs = vehicle.sensorAgeUs;
    state.timestampUs = vehicle.timestampUs;
    state.valid = vehicle.valid;
    state.estimatorValid = vehicle.estimatorValid;
    return state;
}

ActuatorState StatePublisher::buildActuatorState(const RcInputState& rc,
                                                 bool outputsReady,
                                                 bool failsafeActive) const {
    ActuatorState state;
    state.throttle = rc.throttle;
    state.aileron = rc.aileron;
    state.elevator = rc.elevator;
    state.rudder = rc.rudder;
    state.outputsReady = outputsReady;
    state.failsafe = failsafeActive;
    return state;
}
