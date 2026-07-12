#include "AltitudeController.h"

void AltitudeController::init() {
    _targetAltitude = 0.0f;
    _enabled = false;
}

void AltitudeController::update(float currentAltitude, uint16_t throttle, bool failsafe) {
    if (failsafe) {
        _enabled = false;
        return;
    }

    // If not enabled yet, initialize target altitude to current reading
    if (!_enabled) {
        _targetAltitude = currentAltitude;
        _enabled = true;
    }

    // Simple proportional controller stub: compute throttle adjustment (not applied here)
    float error = _targetAltitude - currentAltitude;
    float Kp = 0.1f;
    float adjustment = Kp * error;
    (void)adjustment;
    (void)throttle;
    // Future: output adjusted throttle to motor controller
}

// No global instance — managed by FlightManager
