#ifndef TIMING_MONITOR_H
#define TIMING_MONITOR_H

#include <Arduino.h>
#include "../../types.h"
#include "../../hal/HAL_GPIO.h"
#include "../../hal/HAL_Timer.h"

class TimingMonitor {
  public:
    static constexpr size_t PHASE_COUNT = 4;

    void init(IHALTimer* timer = nullptr, IHALGPIO* gpio = nullptr);
    void begin(size_t phase);
    void end(size_t phase, uint32_t budgetUs, int debugPin = -1);
    bool checkBudgets() const;
    TimingBudgetStatus getStatus() const;
    void resetWindow();

  private:
    static constexpr uint8_t AVG_SHIFT = 3;
    static constexpr uint8_t AVG_Q = 8;

    uint32_t _phaseStartUs[PHASE_COUNT] = {};
    uint32_t _phaseMaxUs[PHASE_COUNT] = {};
    uint32_t _phaseAvgQ8Us[PHASE_COUNT] = {};
    uint32_t _phaseMaxJitterUs[PHASE_COUNT] = {};
    uint16_t _phaseDeadlineMisses[PHASE_COUNT] = {};
    uint8_t _phaseBudgetExceeded[PHASE_COUNT] = {};
    uint16_t _windowSamples = 0;
    IHALTimer* _timer = nullptr;
    IHALGPIO* _gpio = nullptr;

    uint32_t nowUs() const;
};

#endif
