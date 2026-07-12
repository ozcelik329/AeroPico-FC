#ifndef WATCHDOG_GATE_H
#define WATCHDOG_GATE_H

#include <Arduino.h>

struct WatchdogDecision {
    bool shouldFeed;
    const char* reason;
    uint32_t heartbeatAgeUs;
};

class WatchdogGate {
  public:
    static WatchdogDecision evaluate(uint32_t nowUs,
                                     uint32_t flightLoopHeartbeatUs,
                                     bool flightLoopRunning,
                                     bool timingBudgetsOk,
                                     uint32_t staleThresholdUs);
};

#endif
