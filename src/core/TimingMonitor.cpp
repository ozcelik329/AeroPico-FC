#include "TimingMonitor.h"
#include <pico/time.h>
#include <hardware/gpio.h>

void TimingMonitor::init() {
    resetWindow();
}

void TimingMonitor::begin(size_t phase) {
    if (phase >= PHASE_COUNT) {
        return;
    }
    _phaseStartUs[phase] = time_us_32();
}

void TimingMonitor::end(size_t phase, uint32_t budgetUs, int debugPin) {
    if (phase >= PHASE_COUNT) {
        return;
    }

    uint32_t now = time_us_32();
    uint32_t elapsed = now - _phaseStartUs[phase];
    if (elapsed > _phaseMaxUs[phase]) {
        _phaseMaxUs[phase] = elapsed;
    }
    if (elapsed > budgetUs) {
        _phaseBudgetExceeded[phase] = true;
    }
    if (debugPin >= 0) {
        gpio_put((uint)debugPin, 0);
    }
}

bool TimingMonitor::checkBudgets() const {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        if (_phaseBudgetExceeded[i]) {
            return false;
        }
    }
    return true;
}

TimingBudgetStatus TimingMonitor::getStatus() const {
    TimingBudgetStatus status;
    status.consumeUs = _phaseMaxUs[0];
    status.pidUs = _phaseMaxUs[1];
    status.mixerUs = _phaseMaxUs[2];
    status.totalUs = _phaseMaxUs[3];
    status.consumeExceeded = _phaseBudgetExceeded[0];
    status.pidExceeded = _phaseBudgetExceeded[1];
    status.mixerExceeded = _phaseBudgetExceeded[2];
    status.totalExceeded = _phaseBudgetExceeded[3];
    return status;
}

void TimingMonitor::logStats(uint32_t consumeBudgetUs,
                             uint32_t pidBudgetUs,
                             uint32_t mixerBudgetUs,
                             uint32_t totalBudgetUs) const {
    Serial.printf("Timing budgets: consume=%u/%uus pid=%u/%uus mixer=%u/%uus total=%u/%uus\n",
        _phaseMaxUs[0], consumeBudgetUs,
        _phaseMaxUs[1], pidBudgetUs,
        _phaseMaxUs[2], mixerBudgetUs,
        _phaseMaxUs[3], totalBudgetUs);
}

void TimingMonitor::resetWindow() {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        _phaseStartUs[i] = 0;
        _phaseMaxUs[i] = 0;
        _phaseBudgetExceeded[i] = false;
    }
}
