#ifndef RC_PIPELINE_H
#define RC_PIPELINE_H

#include <Arduino.h>
#include "../../drivers/IDrivers.h"
#include "../../types.h"
#include "board/Config.h"

struct RcMapping {
    uint8_t rollChannel;
    uint8_t pitchChannel;
    uint8_t throttleChannel;
    uint8_t yawChannel;
    uint8_t modeChannel;
};

class RCPipeline {
  public:
    void init(IRxDriver* rxDriver);
    RcInputState update();
    RcInputState getState() const;
    void applyMapping(const RcMapping& mapping);
    RcMapping getMapping() const { return _mapping; }

    void setOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder);
    void clearOverride();

  private:
    IRxDriver* _rx = nullptr;
    RcInputState _state = {
        PWM_NEUTRAL,
        PWM_NEUTRAL,
        PWM_MIN,
        PWM_NEUTRAL,
        ControlMode::Manual,
        true,
        false,
        0
    };

    bool _overrideActive = false;
    uint32_t _overrideLastMs = 0;
    uint16_t _overrideAileron = PWM_NEUTRAL;
    uint16_t _overrideElevator = PWM_NEUTRAL;
    uint16_t _overrideThrottle = PWM_MIN;
    uint16_t _overrideRudder = PWM_NEUTRAL;
    RcMapping _mapping = {
        RC_ROLL_CHANNEL,
        RC_PITCH_CHANNEL,
        RC_THROTTLE_CHANNEL,
        RC_YAW_CHANNEL,
        RC_MODE_CHANNEL
    };

    RcInputState failsafeState(uint32_t nowMs) const;
};

#endif
