#ifndef CONTROL_PIPELINE_H
#define CONTROL_PIPELINE_H

#include "../../types.h"
#include "FlightModeController.h"
#include "NavigationController.h"
#include "AltitudeController.h"

struct ControlPipelineInput {
    RcInputState rc;
    VehicleState vehicle;
    bool failsafe;
    bool preflightArmAllowed;
};

class ControlPipeline {
  public:
    void init();
    void update(const ControlPipelineInput& input);
    bool requestArm(bool preflightOk, bool failsafe, uint16_t throttle, const char** reason);
    bool requestDisarm(bool force, uint16_t throttle, const char** reason);
    bool isArmed() const { return _modeController.isArmed(); }
    bool isFailsafe() const { return _modeController.isFailsafe(); }
    FlightState flightState() const { return _modeController.state(); }

  private:
    FlightModeController _modeController;
    NavigationController _navController;
    AltitudeController _altController;
};

#endif
