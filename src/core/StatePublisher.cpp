#include "StatePublisher.h"

FlightData StatePublisher::buildFlightData(const VehicleState& vehicle,
                                           const RcInputState& rc,
                                           const FailsafeDecision& failsafe) const {
    FlightData data;
    data.gyroX = vehicle.gyroX;
    data.gyroY = vehicle.gyroY;
    data.gyroZ = vehicle.gyroZ;
    data.roll = vehicle.rollDeg;
    data.pitch = vehicle.pitchDeg;
    data.yaw = vehicle.yawDeg;
    data.sensorHealth = vehicle.sensorHealth;
    data.timestamp = vehicle.timestampUs;
    data.aileron = rc.aileron;
    data.elevator = rc.elevator;
    data.throttle = rc.throttle;
    data.rudder = rc.rudder;
    data.failsafe = failsafe.active;
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
    state.health = vehicle.sensorHealth;
    state.timestampUs = vehicle.timestampUs;
    state.valid = vehicle.valid;
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
