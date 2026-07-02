#include "FlightModeController.h"

void FlightModeController::init() {
    _armed = false;
    _armHoldStart = 0;
    _disarmHoldStart = 0;
}

void FlightModeController::update(uint16_t throttle, uint16_t rudder) {
    uint32_t now = millis();

    if (!_armed) {
        if (throttle < ARM_THROTTLE_MAX && rudder > ARM_RUDDER_MIN) {
            if (_armHoldStart == 0) _armHoldStart = now;
            if (now - _armHoldStart >= ARM_HOLD_MS) {
                _armed = true;
                _armHoldStart = 0;
                Serial.println("[ARM] Sistem arm edildi!");
            }
        } else {
            _armHoldStart = 0;
        }
    } else {
        if (throttle < ARM_THROTTLE_MAX && rudder < DISARM_RUDDER_MAX) {
            if (_disarmHoldStart == 0) _disarmHoldStart = now;
            if (now - _disarmHoldStart >= ARM_HOLD_MS) {
                _armed = false;
                _disarmHoldStart = 0;
                Serial.println("[ARM] Sistem disarm edildi!");
            }
        } else {
            _disarmHoldStart = 0;
        }
    }
}

// No global instance — managed by FlightManager
