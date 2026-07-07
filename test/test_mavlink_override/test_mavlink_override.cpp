#include <unity.h>

#ifdef MAVLINK_PARAMS_ENABLED
#undef MAVLINK_PARAMS_ENABLED
#endif

#include "telemetry/MavlinkHandler.h"

#include "../../src/telemetry/MavlinkHandler.cpp"

PioUart espUart;

static FlightData latestData;
static uint16_t overrideAileron;
static uint16_t overrideElevator;
static uint16_t overrideThrottle;
static uint16_t overrideRudder;
static bool overrideCalled;
static bool clearCalled;

static bool provideFlightData(FlightData& out) {
    out = latestData;
    return true;
}

static void applyOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder) {
    overrideCalled = true;
    overrideAileron = aileron;
    overrideElevator = elevator;
    overrideThrottle = throttle;
    overrideRudder = rudder;
}

static void clearOverride() {
    clearCalled = true;
}

void setUp() {
    latestData = {};
    latestData.aileron = 1501;
    latestData.elevator = 1502;
    latestData.throttle = 1100;
    latestData.rudder = 1503;
    overrideAileron = overrideElevator = overrideThrottle = overrideRudder = 0;
    overrideCalled = false;
    clearCalled = false;
}

void test_mavlink_override_uses_provided_channels() {
    MavlinkHandler handler;
    handler.setFlightDataProvider(provideFlightData);
    handler.setRCOverrideHandler(applyOverride);
    handler.setClearRCOverrideHandler(clearOverride);

    handler.handleRCOverrideForTest(1600, 1400, 1200, 1700);

    TEST_ASSERT_TRUE(overrideCalled);
    TEST_ASSERT_FALSE(clearCalled);
    TEST_ASSERT_EQUAL_UINT16(1600, overrideAileron);
    TEST_ASSERT_EQUAL_UINT16(1400, overrideElevator);
    TEST_ASSERT_EQUAL_UINT16(1200, overrideThrottle);
    TEST_ASSERT_EQUAL_UINT16(1700, overrideRudder);
}

void test_mavlink_override_ignores_channels_using_latest_data() {
    MavlinkHandler handler;
    handler.setFlightDataProvider(provideFlightData);
    handler.setRCOverrideHandler(applyOverride);

    handler.handleRCOverrideForTest(UINT16_MAX, 1400, UINT16_MAX, 1700);

    TEST_ASSERT_TRUE(overrideCalled);
    TEST_ASSERT_EQUAL_UINT16(1501, overrideAileron);
    TEST_ASSERT_EQUAL_UINT16(1400, overrideElevator);
    TEST_ASSERT_EQUAL_UINT16(1100, overrideThrottle);
    TEST_ASSERT_EQUAL_UINT16(1700, overrideRudder);
}

void test_mavlink_override_all_ignored_clears_override() {
    MavlinkHandler handler;
    handler.setRCOverrideHandler(applyOverride);
    handler.setClearRCOverrideHandler(clearOverride);

    handler.handleRCOverrideForTest(UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX);

    TEST_ASSERT_FALSE(overrideCalled);
    TEST_ASSERT_TRUE(clearCalled);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mavlink_override_uses_provided_channels);
    RUN_TEST(test_mavlink_override_ignores_channels_using_latest_data);
    RUN_TEST(test_mavlink_override_all_ignored_clears_override);
    return UNITY_END();
}
