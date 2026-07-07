#ifndef CONTROL_PIPELINE_H
#define CONTROL_PIPELINE_H

#include "../../types.h"
#include "../FlightModeController.h"
#include "../NavigationController.h"
#include "../AltitudeController.h"

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
    bool isArmed() const { return _modeController.isArmed(); }

  private:
    FlightModeController _modeController;
    NavigationController _navController;
    AltitudeController _altController;
};

#endif
