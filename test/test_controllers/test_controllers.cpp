#include <unity.h>

#include "core/FlightModeController.h"

#include "../../src/core/FlightModeController.cpp"

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
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_flight_mode_controller_starts_disarmed);
    RUN_TEST(test_flight_mode_controller_arms_after_hold);
    RUN_TEST(test_flight_mode_controller_failsafe_disarms);
    RUN_TEST(test_flight_mode_controller_blocks_arm_when_preflight_fails);
    return UNITY_END();
}
