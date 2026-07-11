#ifndef SBUS_MAPPER_H
#define SBUS_MAPPER_H

#include <Arduino.h>
#include "../../config.h"

struct SbusFrameView {
    uint16_t channels[16];
    bool failsafe;
};

class SbusMapper {
  public:
    static uint16_t rawToPwm(uint16_t raw) {
        const int32_t numerator = ((int32_t)raw - 172) * 1000 + 819;
        int32_t pwm = 1000 + numerator / 1639;
        if (pwm < PWM_MIN) return PWM_MIN;
        if (pwm > PWM_MAX) return PWM_MAX;
        return (uint16_t)pwm;
    }

    static void applyFrame(const SbusFrameView& frame, uint16_t* outChannels, size_t outCount) {
        if (!outChannels) {
            return;
        }
        const size_t count = outCount < 16 ? outCount : 16;
        for (size_t i = 0; i < count; ++i) {
            outChannels[i] = rawToPwm(frame.channels[i]);
        }
    }
};

#endif
