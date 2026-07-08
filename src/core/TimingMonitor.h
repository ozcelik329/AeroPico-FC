#ifndef TIMING_MONITOR_H
#define TIMING_MONITOR_H

#include <Arduino.h>
#include "../types.h"

class TimingMonitor {
  public:
    static constexpr size_t PHASE_COUNT = 4;

    void init();
    void begin(size_t phase);
    void end(size_t phase, uint32_t budgetUs, int debugPin = -1);
    bool checkBudgets() const;
    TimingBudgetStatus getStatus() const;
    void logStats(uint32_t consumeBudgetUs,
                  uint32_t pidBudgetUs,
                  uint32_t mixerBudgetUs,
                  uint32_t totalBudgetUs) const;
    void resetWindow();

  private:
    static constexpr uint8_t AVG_SHIFT = 3;
    static constexpr uint8_t AVG_Q = 8;

    volatile uint32_t _phaseStartUs[PHASE_COUNT] = {};
    volatile uint32_t _phaseMaxUs[PHASE_COUNT] = {};
    volatile uint32_t _phaseAvgQ8Us[PHASE_COUNT] = {};
    volatile uint32_t _phaseMaxJitterUs[PHASE_COUNT] = {};
    volatile uint16_t _phaseDeadlineMisses[PHASE_COUNT] = {};
    volatile bool _phaseBudgetExceeded[PHASE_COUNT] = {};
};

#endif
