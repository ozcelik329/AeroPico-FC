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

    const uint32_t sampleQ8 = elapsed << AVG_Q;
    if (_phaseAvgQ8Us[phase] == 0) {
        _phaseAvgQ8Us[phase] = sampleQ8;
    } else {
        _phaseAvgQ8Us[phase] += (sampleQ8 - _phaseAvgQ8Us[phase]) >> AVG_SHIFT;
    }

    const uint32_t avgUs = _phaseAvgQ8Us[phase] >> AVG_Q;
    const uint32_t jitterUs = (elapsed > avgUs) ? (elapsed - avgUs) : (avgUs - elapsed);
    if (jitterUs > _phaseMaxJitterUs[phase]) {
        _phaseMaxJitterUs[phase] = jitterUs;
    }

    if (elapsed > budgetUs) {
        _phaseBudgetExceeded[phase] = true;
        if (_phaseDeadlineMisses[phase] != UINT16_MAX) {
            _phaseDeadlineMisses[phase]++;
        }
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
    status.consumeAvgUs = _phaseAvgQ8Us[0] >> AVG_Q;
    status.pidAvgUs = _phaseAvgQ8Us[1] >> AVG_Q;
    status.mixerAvgUs = _phaseAvgQ8Us[2] >> AVG_Q;
    status.totalAvgUs = _phaseAvgQ8Us[3] >> AVG_Q;
    status.consumeJitterUs = _phaseMaxJitterUs[0];
    status.pidJitterUs = _phaseMaxJitterUs[1];
    status.mixerJitterUs = _phaseMaxJitterUs[2];
    status.totalJitterUs = _phaseMaxJitterUs[3];
    status.consumeDeadlineMisses = _phaseDeadlineMisses[0];
    status.pidDeadlineMisses = _phaseDeadlineMisses[1];
    status.mixerDeadlineMisses = _phaseDeadlineMisses[2];
    status.totalDeadlineMisses = _phaseDeadlineMisses[3];
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
    Serial.printf("Timing budgets: consume=%u avg=%u jit=%u/%uus pid=%u avg=%u jit=%u/%uus mixer=%u avg=%u jit=%u/%uus total=%u avg=%u jit=%u/%uus\n",
        _phaseMaxUs[0], _phaseAvgQ8Us[0] >> AVG_Q, _phaseMaxJitterUs[0], consumeBudgetUs,
        _phaseMaxUs[1], _phaseAvgQ8Us[1] >> AVG_Q, _phaseMaxJitterUs[1], pidBudgetUs,
        _phaseMaxUs[2], _phaseAvgQ8Us[2] >> AVG_Q, _phaseMaxJitterUs[2], mixerBudgetUs,
        _phaseMaxUs[3], _phaseAvgQ8Us[3] >> AVG_Q, _phaseMaxJitterUs[3], totalBudgetUs);
}

void TimingMonitor::resetWindow() {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        _phaseStartUs[i] = 0;
        _phaseMaxUs[i] = 0;
        _phaseAvgQ8Us[i] = 0;
        _phaseMaxJitterUs[i] = 0;
        _phaseDeadlineMisses[i] = 0;
        _phaseBudgetExceeded[i] = false;
    }
}
