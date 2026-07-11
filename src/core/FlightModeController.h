#ifndef FLIGHT_MODE_CONTROLLER_H
#define FLIGHT_MODE_CONTROLLER_H

#include <Arduino.h>
#include "ArmDefs.h"

enum class FlightState : uint8_t {
    Boot = 0,
    Standby,
    PreflightBlocked,
    ReadyToArm,
    ArmedManual,
    Failsafe,
    Safe
};

class FlightModeController {
  public:
    void init();
    void update(uint16_t throttle, uint16_t rudder);
    void update(uint16_t throttle, uint16_t rudder, bool failsafe);
    void update(uint16_t throttle, uint16_t rudder, bool failsafe, bool preflightOk);
    bool isArmed() const { return _state == FlightState::ArmedManual; }
    bool isFailsafe() const { return _state == FlightState::Failsafe; }
    FlightState state() const { return _state; }
    const char* transitionReason() const { return _transitionReason; }

  private:
    FlightState _state = FlightState::Boot;
    const char* _transitionReason = "boot";
    uint32_t _armHoldStart    = 0;
    uint32_t _disarmHoldStart = 0;

    void transitionTo(FlightState state, const char* reason);
};

#endif
