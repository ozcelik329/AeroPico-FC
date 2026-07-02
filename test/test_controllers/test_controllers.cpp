#include <Arduino.h>
#include "native_include/Arduino.h"
#include <unity.h>

#include "core/FlightModeController.h"
#include "core/NavigationController.h"
#include "core/AltitudeController.h"

// Include implementation directly for the test build so symbols are linked
#include "../../src/core/FlightModeController.cpp"

void test_flight_mode_controller_compile() {
    FlightModeController m;
    m.init();
    TEST_ASSERT_FALSE(m.isArmed());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_flight_mode_controller_compile);
    UNITY_END();
}

void loop() {}
