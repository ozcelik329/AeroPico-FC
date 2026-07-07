#include "FixedWingMixer.h"
#include <math.h>
#ifdef UNIT_TEST
static void writeMotors(int, int, int, int) {}
#else
#include "../drivers/Output.h"
#endif

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

static int mapPulse(uint16_t value) {
    long mapped = ((long)value - 1000L) * (PWM_MAX - PWM_MIN) / 1000L + PWM_MIN;
    return constrain((int)mapped, PWM_MIN, PWM_MAX);
}

MixerOutput FixedWingMixer::computeOutputs(uint16_t rawThrottle,
                                           float rollCorrection,
                                           float pitchCorrection,
                                           float yawCorrection,
                                           uint16_t inputAileron,
                                           uint16_t inputElevator,
                                           uint16_t inputRudder) {
    int baseAileron = mapPulse(inputAileron);
    int baseElevator = mapPulse(inputElevator);
    int baseRudder = mapPulse(inputRudder);

    MixerOutput out;
    out.aileron = applyServoMix(baseAileron,
                                rollCorrection * settings.rollGain,
                                settings.aileronTrim);
    out.elevator = applyServoMix(baseElevator,
                                 pitchCorrection * settings.pitchGain,
                                 settings.elevatorTrim);
    out.rudder = applyServoMix(baseRudder,
                               yawCorrection * settings.yawGain,
                               settings.rudderTrim);
    out.throttle = mapThrottle(rawThrottle);
    return out;
}

void FixedWingMixer::compute(uint16_t rawThrottle,
                             float rollCorrection,
                             float pitchCorrection,
                             float yawCorrection,
                             uint16_t inputAileron,
                             uint16_t inputElevator,
                             uint16_t inputRudder) {
    MixerOutput out = computeOutputs(
        rawThrottle,
        rollCorrection,
        pitchCorrection,
        yawCorrection,
        inputAileron,
        inputElevator,
        inputRudder
    );

    writeMotors(out.throttle, out.aileron, out.elevator, out.rudder);
}
