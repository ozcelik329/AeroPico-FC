#include <unity.h>

#include "core/FlightModeController.h"

#include "../../src/core/FlightModeController.cpp"

void test_flight_mode_controller_starts_disarmed() {
    FlightModeController m;
    m.init();
    TEST_ASSERT_FALSE(m.isArmed());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_flight_mode_controller_starts_disarmed);
    return UNITY_END();
}
