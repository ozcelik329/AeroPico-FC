#include "FlightModeController.h"

void FlightModeController::init() {
    _state = FlightState::Standby;
    _transitionReason = "initialized";
    _armHoldStart = 0;
    _disarmHoldStart = 0;
}

void FlightModeController::transitionTo(FlightState state, const char* reason) {
    _state = state;
    _transitionReason = reason;
    _armHoldStart = 0;
    _disarmHoldStart = 0;
}

void FlightModeController::update(uint16_t throttle, uint16_t rudder) {
    update(throttle, rudder, false);
}

void FlightModeController::update(uint16_t throttle, uint16_t rudder, bool failsafe) {
    update(throttle, rudder, failsafe, true);
}

void FlightModeController::update(uint16_t throttle, uint16_t rudder, bool failsafe, bool preflightOk) {
    uint32_t now = millis();

    if (failsafe) {
        transitionTo(FlightState::Failsafe, "failsafe active");
        return;
    }

    if (_state == FlightState::Failsafe || _state == FlightState::Standby) {
        transitionTo(preflightOk ? FlightState::ReadyToArm
                                 : FlightState::PreflightBlocked,
                     preflightOk ? "preflight passed" : "preflight blocked");
    }

    if (_state != FlightState::ArmedManual) {
        if (!preflightOk) {
            if (_state != FlightState::PreflightBlocked) {
                transitionTo(FlightState::PreflightBlocked, "preflight blocked");
            }
            return;
        }
        if (_state != FlightState::ReadyToArm) {
            transitionTo(FlightState::ReadyToArm, "ready to arm");
        }
        if (throttle < ARM_THROTTLE_MAX && rudder >= ARM_RUDDER_MIN) {
            if (_armHoldStart == 0) _armHoldStart = now;
            if (now - _armHoldStart >= ARM_HOLD_MS) {
                transitionTo(FlightState::ArmedManual, "arm gesture");
            }
        } else {
            _armHoldStart = 0;
        }
    } else {
        if (throttle < ARM_THROTTLE_MAX && rudder <= DISARM_RUDDER_MAX) {
            if (_disarmHoldStart == 0) _disarmHoldStart = now;
            if (now - _disarmHoldStart >= ARM_HOLD_MS) {
                transitionTo(FlightState::ReadyToArm, "disarm gesture");
            }
        } else {
            _disarmHoldStart = 0;
        }
    }
}

bool FlightModeController::requestArm(bool preflightOk, bool failsafe, uint16_t throttle, const char** reason) {
    if (isArmed()) {
        if (reason) *reason = "already armed";
        return true;
    }
    if (failsafe) {
        if (reason) *reason = "failsafe active";
        return false;
    }
    if (!preflightOk) {
        if (reason) *reason = "preflight blocked";
        transitionTo(FlightState::PreflightBlocked, "mavlink arm denied");
        return false;
    }
    if (throttle >= ARM_THROTTLE_MAX) {
        if (reason) *reason = "throttle too high";
        return false;
    }

    transitionTo(FlightState::ArmedManual, "mavlink arm");
    if (reason) *reason = "armed";
    return true;
}

bool FlightModeController::requestDisarm(bool force, uint16_t throttle, const char** reason) {
    if (!isArmed()) {
        if (reason) *reason = "already disarmed";
        transitionTo(FlightState::ReadyToArm, "mavlink disarm");
        return true;
    }
    if (!force && throttle >= ARM_THROTTLE_MAX) {
        if (reason) *reason = "throttle too high";
        return false;
    }

    transitionTo(FlightState::ReadyToArm, force ? "mavlink force disarm" : "mavlink disarm");
    if (reason) *reason = "disarmed";
    return true;
}

// No global instance — managed by FlightManager
