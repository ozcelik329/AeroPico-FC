#ifndef FAILSAFE_MANAGER_H
#define FAILSAFE_MANAGER_H

#include "../../types.h"

enum FailsafeReason : uint16_t {
    FailsafeNone = 0,
    FailsafeRcLoss = 1u << 0,
    FailsafeSensorInvalid = 1u << 1,
    FailsafeEstimatorInvalid = 1u << 2,
    FailsafeTiming = 1u << 3,
    FailsafeBatteryCritical = 1u << 4,
    FailsafeActuator = 1u << 5
};

struct FailsafeDecision {
    bool active = false;
    const char* reason = "OK";
    uint16_t reasons = FailsafeNone;
};

class FailsafeManager {
  public:
    void init();
    FailsafeDecision evaluate(const FlightData& data) const;
};

#endif
