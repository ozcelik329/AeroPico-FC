#ifndef CONTROL_LOOP_EXECUTOR_H
#define CONTROL_LOOP_EXECUTOR_H

#include <Arduino.h>
#include "../../hal/HAL_PWM.h"
#include "../../types.h"
#include "../PID.h"
#include "../FixedWingMixer.h"

class FlightManager;

struct ControlCorrections {
    float roll;
    float pitch;
    float yaw;
};

class ControlLoopExecutor {
  public:
    void init(IHALPWM* pwmOutput);
    void resetControllers();
    void applyPidGains(float angleP, float angleI, float angleD,
                       float rateP, float rateI, float rateD);
    void applyMixerSettings(const MixerSettings& settings);
    void writeSafeOutputs();
    ControlCorrections computeCorrections(const FlightData& data, float dt);
    void mixAndWrite(const FlightData& data, const ControlCorrections& corrections);
    bool outputsReady() const { return _pwm != nullptr && _pwmReady; }

  private:
    IHALPWM* _pwm = nullptr;
    bool _pwmReady = false;
    PID _rollAnglePID = PID(0.0f, 0.0f, 0.0f);
    PID _pitchAnglePID = PID(0.0f, 0.0f, 0.0f);
    PID _rollRatePID = PID(0.0f, 0.0f, 0.0f);
    PID _pitchRatePID = PID(0.0f, 0.0f, 0.0f);
    PID _yawRatePID = PID(0.0f, 0.0f, 0.0f);
    FixedWingMixer _mixer;

    void writeOutputs(const MixerOutput& output);
};

#endif
