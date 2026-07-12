#include "TimingMonitor.h"
#include "SystemTimer.h"

void TimingMonitor::init(IHALTimer* timer, IHALGPIO* gpio) {
    _timer = timer;
    _gpio = gpio;
    resetWindow();
}

uint32_t TimingMonitor::nowUs() const {
    return _timer ? _timer->micros() : micros();
}

void TimingMonitor::begin(size_t phase) {
    if (phase >= PHASE_COUNT) {
        return;
    }
    __atomic_store_n(&_phaseStartUs[phase], nowUs(), __ATOMIC_RELEASE);
}

void TimingMonitor::end(size_t phase, uint32_t budgetUs, int debugPin) {
    if (phase >= PHASE_COUNT) {
        return;
    }

    uint32_t now = nowUs();
    uint32_t elapsed = now - __atomic_load_n(&_phaseStartUs[phase], __ATOMIC_ACQUIRE);
    uint32_t maxUs = __atomic_load_n(&_phaseMaxUs[phase], __ATOMIC_RELAXED);
    if (elapsed > maxUs) {
        __atomic_store_n(&_phaseMaxUs[phase], elapsed, __ATOMIC_RELEASE);
    }

    const uint32_t sampleQ8 = elapsed << AVG_Q;
    uint32_t avgQ8 = __atomic_load_n(&_phaseAvgQ8Us[phase], __ATOMIC_RELAXED);
    if (avgQ8 == 0) {
        avgQ8 = sampleQ8;
    } else {
        avgQ8 += (sampleQ8 - avgQ8) >> AVG_SHIFT;
    }
    __atomic_store_n(&_phaseAvgQ8Us[phase], avgQ8, __ATOMIC_RELEASE);

    const uint32_t avgUs = avgQ8 >> AVG_Q;
    const uint32_t jitterUs = (elapsed > avgUs) ? (elapsed - avgUs) : (avgUs - elapsed);
    uint32_t maxJitterUs = __atomic_load_n(&_phaseMaxJitterUs[phase], __ATOMIC_RELAXED);
    if (jitterUs > maxJitterUs) {
        __atomic_store_n(&_phaseMaxJitterUs[phase], jitterUs, __ATOMIC_RELEASE);
    }

    if (elapsed > budgetUs) {
        __atomic_store_n(&_phaseBudgetExceeded[phase], (uint8_t)1, __ATOMIC_RELEASE);
        uint16_t misses = __atomic_load_n(&_phaseDeadlineMisses[phase], __ATOMIC_RELAXED);
        if (misses != UINT16_MAX) {
            __atomic_store_n(&_phaseDeadlineMisses[phase], (uint16_t)(misses + 1U), __ATOMIC_RELEASE);
        }
    }
    if (phase == PHASE_COUNT - 1) {
        uint16_t samples = __atomic_load_n(&_windowSamples, __ATOMIC_RELAXED);
        if (samples != UINT16_MAX) {
            __atomic_store_n(&_windowSamples, (uint16_t)(samples + 1U), __ATOMIC_RELEASE);
        }
    }
    if (debugPin >= 0) {
        if (_gpio) {
            _gpio->write((uint8_t)debugPin, false);
        }
    }
}

bool TimingMonitor::checkBudgets() const {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        if (__atomic_load_n(&_phaseBudgetExceeded[i], __ATOMIC_ACQUIRE) != 0) {
            return false;
        }
    }
    return true;
}

TimingBudgetStatus TimingMonitor::getStatus() const {
    TimingBudgetStatus status;
    status.consumeUs = __atomic_load_n(&_phaseMaxUs[0], __ATOMIC_ACQUIRE);
    status.pidUs = __atomic_load_n(&_phaseMaxUs[1], __ATOMIC_ACQUIRE);
    status.mixerUs = __atomic_load_n(&_phaseMaxUs[2], __ATOMIC_ACQUIRE);
    status.totalUs = __atomic_load_n(&_phaseMaxUs[3], __ATOMIC_ACQUIRE);
    status.consumeAvgUs = __atomic_load_n(&_phaseAvgQ8Us[0], __ATOMIC_ACQUIRE) >> AVG_Q;
    status.pidAvgUs = __atomic_load_n(&_phaseAvgQ8Us[1], __ATOMIC_ACQUIRE) >> AVG_Q;
    status.mixerAvgUs = __atomic_load_n(&_phaseAvgQ8Us[2], __ATOMIC_ACQUIRE) >> AVG_Q;
    status.totalAvgUs = __atomic_load_n(&_phaseAvgQ8Us[3], __ATOMIC_ACQUIRE) >> AVG_Q;
    status.consumeJitterUs = __atomic_load_n(&_phaseMaxJitterUs[0], __ATOMIC_ACQUIRE);
    status.pidJitterUs = __atomic_load_n(&_phaseMaxJitterUs[1], __ATOMIC_ACQUIRE);
    status.mixerJitterUs = __atomic_load_n(&_phaseMaxJitterUs[2], __ATOMIC_ACQUIRE);
    status.totalJitterUs = __atomic_load_n(&_phaseMaxJitterUs[3], __ATOMIC_ACQUIRE);
    status.consumeDeadlineMisses = __atomic_load_n(&_phaseDeadlineMisses[0], __ATOMIC_ACQUIRE);
    status.pidDeadlineMisses = __atomic_load_n(&_phaseDeadlineMisses[1], __ATOMIC_ACQUIRE);
    status.mixerDeadlineMisses = __atomic_load_n(&_phaseDeadlineMisses[2], __ATOMIC_ACQUIRE);
    status.totalDeadlineMisses = __atomic_load_n(&_phaseDeadlineMisses[3], __ATOMIC_ACQUIRE);
    status.totalLoadPermille = status.totalAvgUs >= SystemTimer::LOOP_TIME_US
        ? 1000
        : (uint16_t)((status.totalAvgUs * 1000U) / SystemTimer::LOOP_TIME_US);
    status.windowSamples = __atomic_load_n(&_windowSamples, __ATOMIC_ACQUIRE);
    status.consumeExceeded = __atomic_load_n(&_phaseBudgetExceeded[0], __ATOMIC_ACQUIRE) != 0;
    status.pidExceeded = __atomic_load_n(&_phaseBudgetExceeded[1], __ATOMIC_ACQUIRE) != 0;
    status.mixerExceeded = __atomic_load_n(&_phaseBudgetExceeded[2], __ATOMIC_ACQUIRE) != 0;
    status.totalExceeded = __atomic_load_n(&_phaseBudgetExceeded[3], __ATOMIC_ACQUIRE) != 0;
    return status;
}

void TimingMonitor::resetWindow() {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        __atomic_store_n(&_phaseStartUs[i], 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&_phaseMaxUs[i], 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&_phaseAvgQ8Us[i], 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&_phaseMaxJitterUs[i], 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&_phaseDeadlineMisses[i], (uint16_t)0, __ATOMIC_RELEASE);
        __atomic_store_n(&_phaseBudgetExceeded[i], (uint8_t)0, __ATOMIC_RELEASE);
    }
    __atomic_store_n(&_windowSamples, (uint16_t)0, __ATOMIC_RELEASE);
}
