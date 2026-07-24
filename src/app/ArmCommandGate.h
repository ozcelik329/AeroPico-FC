#pragma once

#include <cstddef>
#include <cstring>

#include "board/Config.h"
#include "core/flight/FlightManager.h"
#include "core/safety/PreflightHealth.h"

namespace ArmCommandGate {

using PreflightEvaluator = PreflightResult (*)();

inline void copyReason(char* reason, size_t reasonLen, const char* text) {
    if (!reason || reasonLen == 0) return;
    reason[0] = '\0';
    if (!text) return;
    strncpy(reason, text, reasonLen - 1);
    reason[reasonLen - 1] = '\0';
}

inline bool request(FlightManager& flightManager,
                    PreflightResult& lastPreflightResult,
                    PreflightEvaluator evaluatePreflight,
                    bool arm,
                    bool force,
                    char* reason,
                    size_t reasonLen) {
    if (arm && force) {
#if AEROPICO_BENCH_FORCE_ARM_ENABLED
        return flightManager.requestBenchForceArm(reason, reasonLen);
#else
        copyReason(reason, reasonLen, "bench force arm disabled");
        return false;
#endif
    }

    if (arm && evaluatePreflight) {
        lastPreflightResult = evaluatePreflight();
        flightManager.setPreflightArmAllowed(lastPreflightResult.canArm);
        if (!lastPreflightResult.canArm) {
            copyReason(reason, reasonLen, lastPreflightResult.firstFailureReason);
            return false;
        }
    }

    const bool accepted = flightManager.requestArmFromMavlink(arm, force, reason, reasonLen);
    if (!accepted && arm && reason && reasonLen > 0 &&
        strncmp(reason, "preflight blocked", reasonLen) == 0 &&
        lastPreflightResult.firstFailureReason[0] != '\0') {
        copyReason(reason, reasonLen, lastPreflightResult.firstFailureReason);
    }
    return accepted;
}

} // namespace ArmCommandGate
