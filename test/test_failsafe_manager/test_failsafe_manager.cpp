#include <unity.h>

#include "core/FailsafeManager.h"

#include "../../src/core/FailsafeManager.cpp"

static FlightData healthyData() {
    FlightData data = {};
    data.failsafe = false;
    data.sensorHealth = SensorHealth::Ok;
    return data;
}

void test_failsafe_manager_allows_healthy_data() {
    FailsafeManager manager;
    manager.init();

    FailsafeDecision decision = manager.evaluate(healthyData());

    TEST_ASSERT_FALSE(decision.active);
}

void test_failsafe_manager_blocks_rc_failsafe() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.failsafe = true;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
}

void test_failsafe_manager_blocks_sensor_timeout() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.sensorHealth = SensorHealth::Timeout;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_failsafe_manager_allows_healthy_data);
    RUN_TEST(test_failsafe_manager_blocks_rc_failsafe);
    RUN_TEST(test_failsafe_manager_blocks_sensor_timeout);
    return UNITY_END();
}
