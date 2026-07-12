#ifndef PREFLIGHT_HEALTH_H
#define PREFLIGHT_HEALTH_H

#include <Arduino.h>

enum class PreflightCheckId : uint8_t {
    Boot = 0,
    Sensor,
    RC,
    GPS,
    Battery,
    Memory,
    Actuator,
    Failsafe,
    Scheduler,
    Count
};

struct PreflightCheck {
    PreflightCheckId id;
    bool required;
    bool passed;
    const char* reason;
};

struct PreflightResult {
    bool canArm;
    const char* firstFailureReason;
    uint8_t failedRequiredCount;
};

class PreflightHealth {
  public:
    static constexpr size_t MAX_CHECKS = (size_t)PreflightCheckId::Count;

    void reset();
    void setCheck(PreflightCheckId id, bool required, bool passed, const char* reason);
    PreflightResult evaluate() const;
    const PreflightCheck* getCheck(PreflightCheckId id) const;

  private:
    PreflightCheck _checks[MAX_CHECKS] = {};
    bool _configured[MAX_CHECKS] = {};
};

#endif
