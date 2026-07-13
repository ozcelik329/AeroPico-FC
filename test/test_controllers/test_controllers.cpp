#include <unity.h>

#include "core/control/FlightModeController.h"

#include "../../src/core/control/FlightModeController.cpp"

void test_flight_mode_controller_starts_disarmed() {
    FlightModeController m;
    m.init();
    TEST_ASSERT_FALSE(m.isArmed());
}

void test_flight_mode_controller_arms_after_hold() {
    FlightModeController m;
    m.init();

    setMockMillis(100);
    m.update(900, 1900, false);
    TEST_ASSERT_FALSE(m.isArmed());

    setMockMillis(100 + ARM_HOLD_MS + 1);
    m.update(900, 1900, false);
    TEST_ASSERT_TRUE(m.isArmed());
}

void test_flight_mode_controller_failsafe_disarms() {
    FlightModeController m;
    m.init();

    setMockMillis(100);
    m.update(900, 1900, false);
    setMockMillis(100 + ARM_HOLD_MS + 1);
    m.update(900, 1900, false);
    TEST_ASSERT_TRUE(m.isArmed());

    m.update(1500, 1500, true);
    TEST_ASSERT_FALSE(m.isArmed());
}

void test_flight_mode_controller_blocks_arm_when_preflight_fails() {
    FlightModeController m;
    m.init();

    setMockMillis(100);
    m.update(900, 1900, false, false);
    setMockMillis(100 + ARM_HOLD_MS + 1);
    m.update(900, 1900, false, false);

    TEST_ASSERT_FALSE(m.isArmed());
    TEST_ASSERT_EQUAL((int)FlightState::PreflightBlocked, (int)m.state());
}

void test_flight_mode_controller_reports_ready_reason_after_preflight_recovers() {
    FlightModeController m;
    m.init();

    m.update(1500, 1500, false, false);
    TEST_ASSERT_EQUAL((int)FlightState::PreflightBlocked, (int)m.state());

    m.update(1500, 1500, false, true);
    TEST_ASSERT_EQUAL((int)FlightState::ReadyToArm, (int)m.state());
    TEST_ASSERT_EQUAL_STRING("ready to arm", m.transitionReason());
}

void test_flight_mode_controller_recovers_from_failsafe_to_ready() {
    FlightModeController m;
    m.init();

    m.update(1500, 1500, true, true);
    TEST_ASSERT_EQUAL((int)FlightState::Failsafe, (int)m.state());

    m.update(1500, 1500, false, true);
    TEST_ASSERT_EQUAL((int)FlightState::ReadyToArm, (int)m.state());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_flight_mode_controller_starts_disarmed);
    RUN_TEST(test_flight_mode_controller_arms_after_hold);
    RUN_TEST(test_flight_mode_controller_failsafe_disarms);
    RUN_TEST(test_flight_mode_controller_blocks_arm_when_preflight_fails);
    RUN_TEST(test_flight_mode_controller_reports_ready_reason_after_preflight_recovers);
    RUN_TEST(test_flight_mode_controller_recovers_from_failsafe_to_ready);
    return UNITY_END();
}
