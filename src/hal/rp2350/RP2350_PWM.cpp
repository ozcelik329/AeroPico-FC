#include "RP2350_PWM.h"
#include "../../drivers/Output.h"

void RP2350PWM::init() {
    outputInit();
}

void RP2350PWM::write(const HALPwmOutputs& outputs) {
    writeMotors(outputs.throttle, outputs.aileron, outputs.elevator, outputs.rudder);
}

bool RP2350PWM::isReady() const {
    return servoOutput.isReady();
}
