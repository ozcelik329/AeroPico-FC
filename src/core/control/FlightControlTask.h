#ifndef FLIGHT_CONTROL_TASK_H
#define FLIGHT_CONTROL_TASK_H

#include "../../types.h"
#include "FixedWingMixer.h"
#include "../scheduling/SystemTimer.h"

class FlightControlTask {
  public:
    static void init();
    static void run();
    static void initTimingMeasurements();
    static void beginTiming(SystemTimer::LoopPhase phase);
    static void endTiming(SystemTimer::LoopPhase phase);
    static bool checkTimingBudgets();
    static TimingBudgetStatus getTimingBudgetStatus();
    static void requestTimingWindowReset();
    static bool outputsReady();
    static bool requestServoTest(uint8_t surface, uint16_t pulseUs, uint16_t durationMs);
    static void applyPidGains(float angleP, float angleI, float angleD,
                              float rateP, float rateI, float rateD);
    static void applyMixerSettings(const MixerSettings& settings);
};

#endif
