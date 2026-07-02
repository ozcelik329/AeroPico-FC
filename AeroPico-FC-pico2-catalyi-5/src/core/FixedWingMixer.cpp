#include "FixedWingMixer.h"
#include <math.h>
#include "../drivers/Output.h"

FixedWingMixer::FixedWingMixer() {
    settings.rollGain = 1.0f;
    settings.pitchGain = 1.0f;
    settings.yawGain = 1.0f;
    settings.aileronTrim = 0;
    settings.elevatorTrim = 0;
    settings.rudderTrim = 0;
    settings.throttleTrim = 0;
}

void FixedWingMixer::init() {
    // outputInit() SystemManager::init() tarafından çağrılıyor,
    // burada tekrar çağırmıyoruz.
}

void FixedWingMixer::setSettings(const MixerSettings& settings) {
    this->settings = settings;
}

int FixedWingMixer::applyServoMix(int baseSignal, float correction, int trim) {
    float scaled = baseSignal + correction;
    int mixed = constrain((int)round(scaled + trim), PWM_MIN, PWM_MAX);
    return mixed;
}

int FixedWingMixer::mapThrottle(uint16_t throttle) {
    return constrain((int)throttle + settings.throttleTrim, PWM_MIN, PWM_MAX);
}

void FixedWingMixer::compute(uint16_t rawThrottle,
                             float rollCorrection,
                             float pitchCorrection,
                             float yawCorrection,
                             uint16_t inputAileron,
                             uint16_t inputElevator,
                             uint16_t inputRudder) {
    int baseAileron = map(inputAileron, 1000, 2000, PWM_MIN, PWM_MAX);
    int baseElevator = map(inputElevator, 1000, 2000, PWM_MIN, PWM_MAX);
    int baseRudder = map(inputRudder, 1000, 2000, PWM_MIN, PWM_MAX);

    int aileronPWM = applyServoMix(baseAileron,
                                  rollCorrection * settings.rollGain,
                                  settings.aileronTrim);
    int elevatorPWM = applyServoMix(baseElevator,
                                   pitchCorrection * settings.pitchGain,
                                   settings.elevatorTrim);
    int rudderPWM = applyServoMix(baseRudder,
                                  yawCorrection * settings.yawGain,
                                  settings.rudderTrim);

    int motorPWM = mapThrottle(rawThrottle);

    writeMotors(motorPWM, aileronPWM, elevatorPWM, rudderPWM);
}