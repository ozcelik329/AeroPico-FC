#include <unity.h>

#include "core/WatchdogGate.h"

#include "../../src/core/WatchdogGate.cpp"

void test_watchdog_gate_feeds_when_flight_loop_is_healthy() {
    WatchdogDecision decision = WatchdogGate::evaluate(
        10000,
        9000,
        true,
        true,
        20000
    );

    TEST_ASSERT_TRUE(decision.shouldFeed);
    TEST_ASSERT_EQUAL_STRING("flight loop healthy", decision.reason);
    TEST_ASSERT_EQUAL_UINT32(1000, decision.heartbeatAgeUs);
}

void test_watchdog_gate_blocks_when_heartbeat_is_stale() {
    WatchdogDecision decision = WatchdogGate::evaluate(
        50000,
        10000,
        true,
        true,
        20000
    );

    TEST_ASSERT_FALSE(decision.shouldFeed);
    TEST_ASSERT_EQUAL_STRING("flight loop heartbeat stale", decision.reason);
}

void test_watchdog_gate_blocks_when_timing_budget_failed() {
    WatchdogDecision decision = WatchdogGate::evaluate(
        10000,
        9000,
        true,
        false,
        20000
    );

    TEST_ASSERT_FALSE(decision.shouldFeed);
    TEST_ASSERT_EQUAL_STRING("flight loop timing budget exceeded", decision.reason);
}

void test_watchdog_gate_blocks_when_flight_loop_not_running() {
    WatchdogDecision decision = WatchdogGate::evaluate(
        10000,
        9000,
        false,
        true,
        20000
    );

    TEST_ASSERT_FALSE(decision.shouldFeed);
    TEST_ASSERT_EQUAL_STRING("flight loop not running", decision.reason);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_watchdog_gate_feeds_when_flight_loop_is_healthy);
    RUN_TEST(test_watchdog_gate_blocks_when_heartbeat_is_stale);
    RUN_TEST(test_watchdog_gate_blocks_when_timing_budget_failed);
    RUN_TEST(test_watchdog_gate_blocks_when_flight_loop_not_running);
    return UNITY_END();
}
