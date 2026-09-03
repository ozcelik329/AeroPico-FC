#include "FlightManager.h"
#include <stdarg.h>

namespace {
void copyReason(char* out, size_t outLen, const char* text) {
    if (!out || outLen == 0) return;
    snprintf(out, outLen, "%s", text ? text : "");
}

void formatReason(char* out, size_t outLen, const char* format, ...) {
    if (!out || outLen == 0) return;
    va_list args;
    va_start(args, format);
    vsnprintf(out, outLen, format, args);
    va_end(args);
}
}

void FlightManager::init() {
    _sensorPipeline.init(nullptr);
    _rcPipeline.init(nullptr);
    _controlPipeline.init();
    _failsafeManager.init();
    _vehicleState = _sensorPipeline.getState();
    _rcState = _rcPipeline.getState();
    __atomic_store_n(&_armedShared, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&_benchForceArmSessionShared, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&_latestFailsafeReasonsShared, FailsafeNone, __ATOMIC_RELEASE);
}

void FlightManager::init(IImuDriver* imuDrv, IRxDriver* rxDrv) {
    _sensorPipeline.init(imuDrv);
    _rcPipeline.init(rxDrv);
    _controlPipeline.init();
    _failsafeManager.init();
    _vehicleState = _sensorPipeline.getState();
    _rcState = _rcPipeline.getState();
    __atomic_store_n(&_armedShared, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&_benchForceArmSessionShared, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&_latestFailsafeReasonsShared, FailsafeNone, __ATOMIC_RELEASE);
}

void FlightManager::attachSensorDriver(IImuDriver* imuDrv) {
    _sensorPipeline.attachImu(imuDrv);
    _vehicleState = _sensorPipeline.getState();
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
    const bool benchAuthorized =
        __atomic_load_n(&_benchForceArmAuthorizedShared, __ATOMIC_ACQUIRE) != 0;
    bool benchSession =
        __atomic_load_n(&_benchForceArmSessionShared, __ATOMIC_ACQUIRE) != 0;
    if (!benchAuthorized && benchSession) {
        benchSession = false;
        __atomic_store_n(&_benchForceArmSessionShared, 0, __ATOMIC_RELEASE);
    }

    FailsafeDecision failsafe = evaluateFailsafe(_vehicleState, _rcState, benchSession);
    __atomic_store_n(&_latestFailsafeReasonsShared, failsafe.reasons, __ATOMIC_RELEASE);
    if (benchSession && failsafe.active) {
        __atomic_store_n(&_benchForceArmSessionShared, 0, __ATOMIC_RELEASE);
    }
    FlightData data = _statePublisher.buildFlightData(_vehicleState, _rcState, failsafe);
    data.timingExceeded = __atomic_load_n(&_timingExceededShared, __ATOMIC_ACQUIRE) != 0;
    data.batteryCritical = __atomic_load_n(&_batteryCriticalShared, __ATOMIC_ACQUIRE) != 0;
    data.actuatorFault = __atomic_load_n(&_actuatorFaultShared, __ATOMIC_ACQUIRE) != 0;

    ControlPipelineInput controlInput;
    controlInput.rc = _rcState;
    controlInput.vehicle = _vehicleState;
    controlInput.failsafe = failsafe.active;
    controlInput.preflightArmAllowed =
        __atomic_load_n(&_preflightArmAllowedShared, __ATOMIC_ACQUIRE) != 0;
    controlInput.rcGesturesEnabled =
        __atomic_load_n(&_rcRequiredShared, __ATOMIC_ACQUIRE) != 0;
    const bool wasArmed = _controlPipeline.isArmed();
    const bool wasFailsafe = _controlPipeline.isFailsafe();
    const FlightState previousState = _controlPipeline.flightState();
    _controlPipeline.update(controlInput);
    const FlightState currentState = _controlPipeline.flightState();
    if (wasArmed && !_controlPipeline.isArmed()) {
        __atomic_store_n(&_benchForceArmSessionShared, 0u, __ATOMIC_RELEASE);
    }
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
            (uint32_t)(_controlPipeline.isFailsafe() ? failsafe.reasons : FailsafeNone)
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

void FlightManager::setDefaultControlMode(ControlMode mode) {
    _rcPipeline.setDefaultControlMode(mode);
}

void FlightManager::setPreflightArmAllowed(bool allowed) {
    __atomic_store_n(&_preflightArmAllowedShared, allowed ? 1u : 0u, __ATOMIC_RELEASE);
}

void FlightManager::setBenchForceArmAllowed(bool allowed) {
    __atomic_store_n(&_benchForceArmAuthorizedShared, allowed ? 1u : 0u, __ATOMIC_RELEASE);
    if (!allowed) {
        __atomic_store_n(&_benchForceArmSessionShared, 0, __ATOMIC_RELEASE);
    }
}

void FlightManager::setRcRequired(bool required) {
    __atomic_store_n(&_rcRequiredShared, required ? 1u : 0u, __ATOMIC_RELEASE);
}

bool FlightManager::requestArmFromMavlink(bool arm, bool force, char* reason, size_t reasonLen) {
    const char* controllerReason = "";
    bool accepted = false;
    VehicleState vehicle = _vehicleState;
    RcInputState rc = _rcState;
    systemBlackboard.vehicle.read(vehicle);
    systemBlackboard.rc.read(rc);

    const bool rcRequired = __atomic_load_n(&_rcRequiredShared, __ATOMIC_ACQUIRE) != 0;
    const bool benchAuthorized =
        __atomic_load_n(&_benchForceArmAuthorizedShared, __ATOMIC_ACQUIRE) != 0;
    const bool preflightAllowed =
        __atomic_load_n(&_preflightArmAllowedShared, __ATOMIC_ACQUIRE) != 0;
    const uint16_t throttle = rcRequired ? rc.throttle : PWM_MIN;
    const FailsafeDecision decision = evaluateFailsafe(vehicle, rc, arm && force);
    __atomic_store_n(&_latestFailsafeReasonsShared, decision.reasons, __ATOMIC_RELEASE);

    if (arm && force) {
        if (!benchAuthorized) {
            copyReason(reason, reasonLen, "ARM_DENIED BENCH_JUMPER");
        } else if (decision.active) {
            formatReason(reason, reasonLen, "ARM_DENIED %s MASK=0x%02X",
                         decision.reason, decision.reasons);
        } else {
            accepted = _controlPipeline.forceArm(&controllerReason);
            if (accepted) {
                __atomic_store_n(&_benchForceArmSessionShared, 1u, __ATOMIC_RELEASE);
                copyReason(reason, reasonLen, "ARMED BENCH_FORCE");
            }
        }
    } else if (arm) {
        __atomic_store_n(&_benchForceArmSessionShared, 0u, __ATOMIC_RELEASE);
        if (decision.active) {
            formatReason(reason, reasonLen, "ARM_DENIED %s MASK=0x%02X",
                         decision.reason, decision.reasons);
        } else {
            accepted = _controlPipeline.requestArm(
                preflightAllowed, false, throttle, &controllerReason);
            if (accepted) {
                copyReason(reason, reasonLen, "ARMED NORMAL");
            } else {
                formatReason(reason, reasonLen, "ARM_DENIED %s",
                             preflightAllowed ? controllerReason : "PREFLIGHT");
            }
        }
    } else {
        accepted = _controlPipeline.requestDisarm(force, throttle, &controllerReason);
        if (accepted) {
            __atomic_store_n(&_benchForceArmSessionShared, 0u, __ATOMIC_RELEASE);
            copyReason(reason, reasonLen, "DISARMED");
        } else {
            formatReason(reason, reasonLen, "DISARM_DENIED %s", controllerReason);
        }
    }

    __atomic_store_n(&_armedShared, _controlPipeline.isArmed() ? 1u : 0u, __ATOMIC_RELEASE);
    systemEventBus.publish({
        accepted ? SystemEventType::ArmStateChanged : SystemEventType::ArmDenied,
        vehicle.timestampUs,
        accepted ? (_controlPipeline.isArmed() ? 1u : 0u) : decision.reasons
    });
    return accepted;
}

void FlightManager::setSystemFaults(bool timingExceeded, bool batteryCritical, bool actuatorFault) {
    __atomic_store_n(&_timingExceededShared, timingExceeded ? 1u : 0u, __ATOMIC_RELEASE);
    __atomic_store_n(&_batteryCriticalShared, batteryCritical ? 1u : 0u, __ATOMIC_RELEASE);
    __atomic_store_n(&_actuatorFaultShared, actuatorFault ? 1u : 0u, __ATOMIC_RELEASE);
}

FailsafeDecision FlightManager::evaluateFailsafe(const VehicleState& vehicle,
                                                 const RcInputState& rc,
                                                 bool applyBenchPolicy) const {
    FailsafeDecision none = {};
    FlightData data = _statePublisher.buildFlightData(vehicle, rc, none);
    data.timingExceeded = __atomic_load_n(&_timingExceededShared, __ATOMIC_ACQUIRE) != 0;
    data.batteryCritical = __atomic_load_n(&_batteryCriticalShared, __ATOMIC_ACQUIRE) != 0;
    data.actuatorFault = __atomic_load_n(&_actuatorFaultShared, __ATOMIC_ACQUIRE) != 0;

    FailsafePolicy policy;
    policy.rcRequired = __atomic_load_n(&_rcRequiredShared, __ATOMIC_ACQUIRE) != 0;
    policy.bypassMask = applyBenchPolicy ? FAILSAFE_BENCH_BYPASS_MASK : FailsafeNone;
    return _failsafeManager.evaluate(data, policy);
}
