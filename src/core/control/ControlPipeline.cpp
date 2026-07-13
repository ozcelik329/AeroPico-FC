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

bool ControlPipeline::requestArm(bool preflightOk, bool failsafe, uint16_t throttle, const char** reason) {
    return _modeController.requestArm(preflightOk, failsafe, throttle, reason);
}

bool ControlPipeline::requestDisarm(bool force, uint16_t throttle, const char** reason) {
    return _modeController.requestDisarm(force, throttle, reason);
}
