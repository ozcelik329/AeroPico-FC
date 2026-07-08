#include "FlightManager.h"

void FlightManager::init() {
    _sensorPipeline.init(nullptr);
    _rcPipeline.init(nullptr);
    _controlPipeline.init();
    _failsafeManager.init();
    _vehicleState = _sensorPipeline.getState();
    _rcState = _rcPipeline.getState();
}

void FlightManager::init(IImuDriver* imuDrv, IRxDriver* rxDrv) {
    _sensorPipeline.init(imuDrv);
    _rcPipeline.init(rxDrv);
    _controlPipeline.init();
    _failsafeManager.init();
    _vehicleState = _sensorPipeline.getState();
    _rcState = _rcPipeline.getState();
}

void FlightManager::update() {
    updateSensors();
    updateRc();
    publishState();
}

void FlightManager::updateSensors() {
    _vehicleState = _sensorPipeline.update();
}

void FlightManager::updateRc() {
    _rcState = _rcPipeline.update();
}

void FlightManager::publishState() {
    FlightData provisional = _statePublisher.buildFlightData(_vehicleState, _rcState, {_rcState.failsafe, "RC pipeline"});
    FailsafeDecision failsafe = _failsafeManager.evaluate(provisional);
    FlightData data = _statePublisher.buildFlightData(_vehicleState, _rcState, failsafe);

    ControlPipelineInput controlInput;
    controlInput.rc = _rcState;
    controlInput.vehicle = _vehicleState;
    controlInput.failsafe = failsafe.active;
    controlInput.preflightArmAllowed = _preflightArmAllowed;
    _controlPipeline.update(controlInput);

    _ringBuf.push(data);
}

// Consumer: pop all pending items and update the cached `_latest` (called by single consumer Core 1)
void FlightManager::consumeLatest() {
    FlightData tmp;
    while (_ringBuf.pop(tmp)) {
        // mark write in progress (odd)
        _latest_seq++;
        _latest = tmp;
        __dmb();
        // mark write done (even)
        _latest_seq++;
    }
}

// Peek the most recent item without consuming (safe for telemetry on Core 0)
bool FlightManager::peekLatest(FlightData& out) const {
    return _ringBuf.peek(out);
}

// Read a consistent snapshot of `_latest` using sequence counter
FlightData FlightManager::readLatestSnapshot() const {
    FlightData copy;
    while (true) {
        uint32_t s1 = _latest_seq;
        if (s1 & 1) continue; // writer in progress
        copy = _latest;
        uint32_t s2 = _latest_seq;
        if (s1 == s2) break;
    }
    return copy;
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
