#include <unity.h>

#include "core/FailsafeManager.h"

#include "../../src/core/FailsafeManager.cpp"

static FlightData healthyData() {
    FlightData data = {};
    data.failsafe = false;
    data.sensorHealth = SensorHealth::Ok;
    data.estimatorHealth = SensorHealth::Ok;
    data.estimatorValid = true;
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
    TEST_ASSERT_BITS(FailsafeRcLoss, FailsafeRcLoss, decision.reasons);
}

void test_failsafe_manager_blocks_invalid_estimator() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.estimatorValid = false;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
    TEST_ASSERT_BITS(FailsafeEstimatorInvalid, FailsafeEstimatorInvalid, decision.reasons);
}

void test_failsafe_manager_blocks_sensor_timeout() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.sensorHealth = SensorHealth::Timeout;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
}

void test_failsafe_manager_blocks_sensor_stale() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.sensorHealth = SensorHealth::Stale;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
}

void test_failsafe_manager_blocks_timing_fault() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.timingExceeded = true;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
    TEST_ASSERT_BITS(FailsafeTiming, FailsafeTiming, decision.reasons);
}

void test_failsafe_manager_blocks_battery_critical() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.batteryCritical = true;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
    TEST_ASSERT_BITS(FailsafeBatteryCritical, FailsafeBatteryCritical, decision.reasons);
}

void test_failsafe_manager_blocks_actuator_fault() {
    FailsafeManager manager;
    FlightData data = healthyData();
    data.actuatorFault = true;

    FailsafeDecision decision = manager.evaluate(data);

    TEST_ASSERT_TRUE(decision.active);
    TEST_ASSERT_BITS(FailsafeActuator, FailsafeActuator, decision.reasons);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_failsafe_manager_allows_healthy_data);
    RUN_TEST(test_failsafe_manager_blocks_rc_failsafe);
    RUN_TEST(test_failsafe_manager_blocks_sensor_timeout);
    RUN_TEST(test_failsafe_manager_blocks_sensor_stale);
    RUN_TEST(test_failsafe_manager_blocks_invalid_estimator);
    RUN_TEST(test_failsafe_manager_blocks_timing_fault);
    RUN_TEST(test_failsafe_manager_blocks_battery_critical);
    RUN_TEST(test_failsafe_manager_blocks_actuator_fault);
    return UNITY_END();
}
