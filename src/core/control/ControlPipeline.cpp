#include "ControlPipeline.h"

void ControlPipeline::init() {
    _modeController.init();
    _navController.init();
    _altController.init();
}

void ControlPipeline::update(const ControlPipelineInput& input) {
    _modeController.update(
        input.rc.throttle,
        input.rc.rudder,
        input.failsafe,
        input.preflightArmAllowed
    );

    _navController.update(input.rc.aileron, input.rc.elevator, input.failsafe);
    _altController.update(0.0f, input.rc.throttle, input.failsafe);
}
