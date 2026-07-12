#include <unity.h>

#include "core/safety/BatteryMonitor.h"
#include "estimators/BaroVerticalKalman.h"

static bool provideLinkedVoltage(float& voltage) {
    voltage = 11.7f;
    return true;
}

void test_native_full_link_uses_separate_translation_units() {
    BatteryMonitor battery;
    battery.init(provideLinkedVoltage, 10.5f, 12.8f, 9.0f);
    BatteryStatus status = battery.evaluate();

    BaroVerticalKalman estimator;
    estimator.init();
    EstimatorInput input = {};
    input.rollDeg = 0.0f;
    input.pitchDeg = 0.0f;
    input.yawDeg = 0.0f;
    input.timestampUs = 1000000;
    input.sensorHealth = SensorHealth::Ok;
    EstimatedState state = estimator.update(input, 42.0f, true);

    TEST_ASSERT_TRUE(status.healthy);
    TEST_ASSERT_TRUE(state.valid);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_native_full_link_uses_separate_translation_units);
    return UNITY_END();
}
