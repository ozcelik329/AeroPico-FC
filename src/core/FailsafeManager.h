#ifndef FAILSAFE_MANAGER_H
#define FAILSAFE_MANAGER_H

#include "../types.h"

struct FailsafeDecision {
    bool active;
    const char* reason;
};

class FailsafeManager {
  public:
    void init();
    FailsafeDecision evaluate(const FlightData& data) const;
};

#endif
