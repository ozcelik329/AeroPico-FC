#include "WatchdogGate.h"

WatchdogDecision WatchdogGate::evaluate(uint32_t nowUs,
                                        uint32_t flightLoopHeartbeatUs,
                                        bool flightLoopRunning,
                                        bool timingBudgetsOk,
                                        uint32_t staleThresholdUs) {
    WatchdogDecision decision;
    decision.shouldFeed = false;
    decision.reason = "";
    decision.heartbeatAgeUs = nowUs - flightLoopHeartbeatUs;

    if (!flightLoopRunning) {
        decision.reason = "flight loop not running";
        return decision;
    }

    if (flightLoopHeartbeatUs == 0 || decision.heartbeatAgeUs >= staleThresholdUs) {
        decision.reason = "flight loop heartbeat stale";
        return decision;
    }

    decision.shouldFeed = true;
    decision.reason = timingBudgetsOk ? "flight loop healthy" : "flight loop timing warning";
    return decision;
}
