#ifndef FLIGHT_MODE_CONTROLLER_H
#define FLIGHT_MODE_CONTROLLER_H

#include <Arduino.h>
#include "ArmDefs.h"

class FlightModeController {
  public:
    void init();
    void update(uint16_t throttle, uint16_t rudder);
    void update(uint16_t throttle, uint16_t rudder, bool failsafe);
    void update(uint16_t throttle, uint16_t rudder, bool failsafe, bool preflightOk);
    bool isArmed() const { return _armed; }

  private:
    bool     _armed           = false;
    uint32_t _armHoldStart    = 0;
    uint32_t _disarmHoldStart = 0;
};

#endif
