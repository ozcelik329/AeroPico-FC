#define UNIT_TEST
#include <Arduino.h>
#include <unity.h>

#include "core/FlightModeController.h"
#include "core/NavigationController.h"
#include "core/AltitudeController.h"
#include "core/SystemTimer.h"

// Include implementation directly for the test build so symbols are linked
#include "../../src/core/FlightModeController.cpp"
#include "../../src/core/SystemTimer.cpp"

void test_flight_mode_controller_compile() {
    FlightModeController m;
    m.init();
    TEST_ASSERT_FALSE(m.isArmed());
}

void test_timing_budget_constants() {
    TEST_ASSERT_EQUAL_UINT32(SystemTimer::PHASE_CONSUME_BUDGET_US, 150);
    TEST_ASSERT_EQUAL_UINT32(SystemTimer::PHASE_PID_BUDGET_US, 650);
    TEST_ASSERT_EQUAL_UINT32(SystemTimer::PHASE_MIXER_BUDGET_US, 300);
    TEST_ASSERT_EQUAL_UINT32(SystemTimer::PHASE_TOTAL_BUDGET_US, 1500);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_flight_mode_controller_compile);
    RUN_TEST(test_timing_budget_constants);
    UNITY_END();
}

void loop() {}
