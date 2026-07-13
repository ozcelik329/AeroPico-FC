#ifndef FIXED_WING_MIXER_H
#define FIXED_WING_MIXER_H

#include <Arduino.h>
#include "../../config.h"

struct MixerSettings {
    float rollGain;
    float pitchGain;
    float yawGain;
    int aileronTrim;
    int elevatorTrim;
    int rudderTrim;
    int throttleTrim;
    int servoMin;
    int servoMax;
    bool reverseAileron;
    bool reverseElevator;
    bool reverseRudder;
};

struct MixerOutput {
    int throttle;
    int aileron;
    int elevator;
    int rudder;
};

class FixedWingMixer {
  public:
    FixedWingMixer();
    void init();
    void setSettings(const MixerSettings& settings);
    MixerOutput computeOutputs(uint16_t rawThrottle,
                               float rollCorrection,
                               float pitchCorrection,
                               float yawCorrection,
                               uint16_t inputAileron,
                               uint16_t inputElevator,
                               uint16_t inputRudder);

  private:
    MixerSettings settings;
    int applyServoMix(int baseSignal, float correction, int trim);
    int mapThrottle(uint16_t throttle);
};

#endif
