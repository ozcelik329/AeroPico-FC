#include "FlightManager.h"

void FlightManager::init() {
    _sensorPipeline.init(nullptr);
    _rcPipeline.init(nullptr);
    _controlPipeline.init();
    _failsafeManager.init();
    _vehicleState = _sensorPipeline.getState();
    _rcState = _rcPipeline.getState();
    __atomic_store_n(&_armedShared, 0, __ATOMIC_RELEASE);
}

void FlightManager::init(IImuDriver* imuDrv, IRxDriver* rxDrv) {
    _sensorPipeline.init(imuDrv);
    _rcPipeline.init(rxDrv);
    _controlPipeline.init();
    _failsafeManager.init();
    _vehicleState = _sensorPipeline.getState();
    _rcState = _rcPipeline.getState();
    __atomic_store_n(&_armedShared, 0, __ATOMIC_RELEASE);
}

void FlightManager::update() {
    updateSensors();
    updateRc();
    publishState();
}

void FlightManager::updateSensors() {
    _vehicleState = _sensorPipeline.update();
    systemBlackboard.vehicle.publish(_vehicleState);
    systemBlackboard.sensor.publish(_statePublisher.buildSensorState(_vehicleState));
    if (_vehicleState.sensorHealth != _lastPublishedSensorHealth) {
        _lastPublishedSensorHealth = _vehicleState.sensorHealth;
        if (_vehicleState.sensorHealth != SensorHealth::Ok &&
            _vehicleState.sensorHealth != SensorHealth::WarmingUp) {
            systemEventBus.publish({
                SystemEventType::SensorFault,
                _vehicleState.timestampUs,
                (uint32_t)_vehicleState.sensorHealth
            });
        }
    }
}

void FlightManager::updateRc() {
    const bool previousFailsafe = _rcState.failsafe;
    _rcState = _rcPipeline.update();
    systemBlackboard.rc.publish(_rcState);
    if (previousFailsafe != _rcState.failsafe) {
        systemEventBus.publish({
            _rcState.failsafe ? SystemEventType::RcLost : SystemEventType::RcRecovered,
            _vehicleState.timestampUs,
            0
        });
    }
}

void FlightManager::publishState() {
    FlightData provisional = _statePublisher.buildFlightData(_vehicleState, _rcState, {_rcState.failsafe, "RC pipeline"});
    provisional.timingExceeded = _timingExceeded;
    provisional.batteryCritical = _batteryCritical;
    provisional.actuatorFault = _actuatorFault;
    FailsafeDecision failsafe = _failsafeManager.evaluate(provisional);
    FlightData data = _statePublisher.buildFlightData(_vehicleState, _rcState, failsafe);
    data.timingExceeded = _timingExceeded;
    data.batteryCritical = _batteryCritical;
    data.actuatorFault = _actuatorFault;

    ControlPipelineInput controlInput;
    controlInput.rc = _rcState;
    controlInput.vehicle = _vehicleState;
    controlInput.failsafe = failsafe.active;
    controlInput.preflightArmAllowed = _preflightArmAllowed;
    const bool wasArmed = _controlPipeline.isArmed();
    const bool wasFailsafe = _controlPipeline.isFailsafe();
    const FlightState previousState = _controlPipeline.flightState();
    _controlPipeline.update(controlInput);
    const FlightState currentState = _controlPipeline.flightState();
    __atomic_store_n(&_armedShared, _controlPipeline.isArmed() ? 1u : 0u, __ATOMIC_RELEASE);
    if (wasArmed != _controlPipeline.isArmed()) {
        systemEventBus.publish({SystemEventType::ArmStateChanged, data.timestamp,
                                _controlPipeline.isArmed() ? 1u : 0u});
    }
    if (wasFailsafe != _controlPipeline.isFailsafe()) {
        systemEventBus.publish({
            _controlPipeline.isFailsafe() ? SystemEventType::FailsafeEntered
                                          : SystemEventType::FailsafeCleared,
            data.timestamp,
            0
        });
    }
    if (previousState != FlightState::PreflightBlocked &&
        currentState == FlightState::PreflightBlocked) {
        systemEventBus.publish({SystemEventType::ArmDenied, data.timestamp, failsafe.reasons});
    }

    systemBlackboard.telemetry.publish(data);
}

bool FlightManager::consumeLatest() {
    return systemBlackboard.telemetry.read(_latest);
}

bool FlightManager::consumeLatest(FlightData& out) {
    if (!consumeLatest()) {
        return false;
    }
    out = _latest;
    return true;
}

bool FlightManager::peekLatest(FlightData& out) const {
    return systemBlackboard.telemetry.read(out);
}

float    FlightManager::getRoll()     { return readLatestSnapshot().roll; }
float    FlightManager::getPitch()    { return readLatestSnapshot().pitch; }
float    FlightManager::getYaw()      { return readLatestSnapshot().yaw; }
float    FlightManager::getGyroX()    { return readLatestSnapshot().gyroX; }
float    FlightManager::getGyroY()    { return readLatestSnapshot().gyroY; }
float    FlightManager::getGyroZ()    { return readLatestSnapshot().gyroZ; }
uint16_t FlightManager::getAileron()  { return readLatestSnapshot().aileron; }
uint16_t FlightManager::getElevator() { return readLatestSnapshot().elevator; }
uint16_t FlightManager::getThrottle() { return readLatestSnapshot().throttle; }
uint16_t FlightManager::getRudder()   { return readLatestSnapshot().rudder; }

void FlightManager::setRCOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder) {
    _rcPipeline.setOverride(aileron, elevator, throttle, rudder);
}

void FlightManager::clearRCOverride() {
    _rcPipeline.clearOverride();
}

void FlightManager::applyRcMapping(const RcMapping& mapping) {
    _rcPipeline.applyMapping(mapping);
}

void FlightManager::setPreflightArmAllowed(bool allowed) {
    _preflightArmAllowed = allowed;
}

void FlightManager::setSystemFaults(bool timingExceeded, bool batteryCritical, bool actuatorFault) {
    _timingExceeded = timingExceeded;
    _batteryCritical = batteryCritical;
    _actuatorFault = actuatorFault;
}
