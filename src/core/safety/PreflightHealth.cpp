#include "PreflightHealth.h"

void PreflightHealth::reset() {
    for (size_t i = 0; i < MAX_CHECKS; ++i) {
        _checks[i] = {};
        _checks[i].id = (PreflightCheckId)i;
        _checks[i].required = true;
        _checks[i].passed = false;
        _checks[i].reason = "Check not configured";
        _configured[i] = false;
    }
}

void PreflightHealth::setCheck(PreflightCheckId id, bool required, bool passed, const char* reason) {
    size_t index = (size_t)id;
    if (index >= MAX_CHECKS) {
        return;
    }

    _checks[index].id = id;
    _checks[index].required = required;
    _checks[index].passed = passed;
    _checks[index].reason = reason ? reason : "";
    _configured[index] = true;
}

PreflightResult PreflightHealth::evaluate() const {
    PreflightResult result;
    result.canArm = true;
    result.firstFailureReason = "";
    result.failedRequiredCount = 0;

    for (size_t i = 0; i < MAX_CHECKS; ++i) {
        if (!_configured[i]) {
            continue;
        }

        if (_checks[i].required && !_checks[i].passed) {
            result.canArm = false;
            result.failedRequiredCount++;
            if (result.firstFailureReason[0] == '\0') {
                result.firstFailureReason = _checks[i].reason;
            }
        }
    }

    return result;
}

const PreflightCheck* PreflightHealth::getCheck(PreflightCheckId id) const {
    size_t index = (size_t)id;
    if (index >= MAX_CHECKS || !_configured[index]) {
        return nullptr;
    }
    return &_checks[index];
}
