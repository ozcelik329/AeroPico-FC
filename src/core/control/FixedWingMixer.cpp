#include "FixedWingMixer.h"
#include "../../utils/FastMath.h"
#include <math.h>

FixedWingMixer::FixedWingMixer() {
    settings.rollGain = 1.0f;
    settings.pitchGain = 1.0f;
    settings.yawGain = 1.0f;
    settings.aileronTrim = 0;
    settings.elevatorTrim = 0;
    settings.rudderTrim = 0;
    settings.throttleTrim = 0;
    settings.servoMin = PWM_MIN;
    settings.servoMax = PWM_MAX;
    settings.reverseAileron = false;
    settings.reverseElevator = false;
    settings.reverseRudder = false;
}

void FixedWingMixer::init() {
    // outputInit() SystemManager::init() tarafından çağrılıyor,
    // burada tekrar çağırmıyoruz.
}

void FixedWingMixer::setSettings(const MixerSettings& settings) {
    this->settings = settings;
    if (this->settings.servoMin < PWM_MIN) this->settings.servoMin = PWM_MIN;
    if (this->settings.servoMax > PWM_MAX) this->settings.servoMax = PWM_MAX;
    if (this->settings.servoMin > this->settings.servoMax) {
        int tmp = this->settings.servoMin;
        this->settings.servoMin = this->settings.servoMax;
        this->settings.servoMax = tmp;
    }
}

int FixedWingMixer::applyServoMix(int baseSignal, float correction, int trim) {
    float scaled = baseSignal + correction;
    int mixed = constrain((int)round(scaled + trim), settings.servoMin, settings.servoMax);
    return mixed;
}

int FixedWingMixer::mapThrottle(uint16_t throttle) {
    return constrain((int)throttle + settings.throttleTrim, PWM_MIN, PWM_MAX);
}

static int mapPulse(uint16_t value) {
    return AeroPicoFastMath::pwmToRange(value, PWM_MIN, PWM_MAX);
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
    if (settings.reverseAileron) {
        out.aileron = constrain(PWM_NEUTRAL - (out.aileron - PWM_NEUTRAL), settings.servoMin, settings.servoMax);
    }
    if (settings.reverseElevator) {
        out.elevator = constrain(PWM_NEUTRAL - (out.elevator - PWM_NEUTRAL), settings.servoMin, settings.servoMax);
    }
    if (settings.reverseRudder) {
        out.rudder = constrain(PWM_NEUTRAL - (out.rudder - PWM_NEUTRAL), settings.servoMin, settings.servoMax);
    }
    out.throttle = mapThrottle(rawThrottle);
    return out;
}
