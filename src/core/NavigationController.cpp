#include "NavigationController.h"

void NavigationController::init() {
    _active = false;
}

void NavigationController::update(uint16_t aileron, uint16_t elevator, bool failsafe) {
    if (failsafe) {
        _active = false;
        return;
    }

    // Simple activation logic: if sticks moved away from neutral, enable navigation pass-through
    const uint16_t DEADZONE = 10;
    const uint16_t NEUTRAL = 1500; // assume PWM_NEUTRAL

    if (abs((int)aileron - (int)NEUTRAL) > DEADZONE || abs((int)elevator - (int)NEUTRAL) > DEADZONE) {
        _active = true;
    }

    // Future: translate pilot sticks to navigation setpoints when autopilot engaged
}

// No global instance — managed by FlightManager
