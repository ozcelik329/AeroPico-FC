#include <unity.h>

#include "core/scheduling/TimingMonitor.h"
#include "core/scheduling/SystemTimer.h"

#include "../../src/core/scheduling/TimingMonitor.cpp"

class FakeTimer : public IHALTimer {
  public:
    uint32_t micros() const override { return nowUs; }
    uint32_t millis() const override { return nowUs / 1000U; }
    void delayMs(uint32_t ms) override { advance(ms * 1000U); }
    void advance(uint32_t deltaUs) { nowUs += deltaUs; }
    void set(uint32_t valueUs) { nowUs = valueUs; }

  private:
    uint32_t nowUs = 0;
};

void test_average_decreases_without_unsigned_underflow() {
    FakeTimer timer;
    TimingMonitor monitor;
    monitor.init(&timer, nullptr);

    monitor.begin(SystemTimer::PHASE_TOTAL);
    timer.advance(1000);
    monitor.end(SystemTimer::PHASE_TOTAL, SystemTimer::PHASE_TOTAL_BUDGET_US);

    for (int i = 0; i < 12; ++i) {
        monitor.begin(SystemTimer::PHASE_TOTAL);
        timer.advance(100);
        monitor.end(SystemTimer::PHASE_TOTAL, SystemTimer::PHASE_TOTAL_BUDGET_US);
    }

    TimingBudgetStatus status = monitor.getStatus();
    TEST_ASSERT_LESS_THAN_UINT32(1000, status.totalAvgUs);
    TEST_ASSERT_GREATER_THAN_UINT32(90, status.totalAvgUs);
    TEST_ASSERT_FALSE(status.totalExceeded);
}

void test_budget_and_deadline_counters_remain_functional() {
    FakeTimer timer;
    TimingMonitor monitor;
    monitor.init(&timer, nullptr);

    monitor.begin(SystemTimer::PHASE_PID);
    timer.advance(SystemTimer::PHASE_PID_BUDGET_US + 10);
    monitor.end(SystemTimer::PHASE_PID, SystemTimer::PHASE_PID_BUDGET_US);

    TimingBudgetStatus status = monitor.getStatus();
    TEST_ASSERT_TRUE(status.pidExceeded);
    TEST_ASSERT_EQUAL_UINT16(1, status.pidDeadlineMisses);
    TEST_ASSERT_FALSE(monitor.checkBudgets());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_average_decreases_without_unsigned_underflow);
    RUN_TEST(test_budget_and_deadline_counters_remain_functional);
    return UNITY_END();
}
