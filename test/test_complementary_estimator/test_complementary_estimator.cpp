#include <unity.h>

#include "estimators/ComplementaryEstimator.h"

#include "../../src/estimators/ComplementaryEstimator.cpp"

static EstimatorInput makeEstimatorInput(uint32_t timestamp, SensorHealth health = SensorHealth::Ok, bool failsafe = false) {
    EstimatorInput input = {};
    input.rollDeg = 10.0f;
    input.pitchDeg = -2.0f;
    input.yawDeg = 45.0f;
    input.timestampUs = timestamp;
    input.sensorHealth = health;
    input.failsafe = failsafe;
    return input;
}

void test_estimator_copies_attitude() {
    ComplementaryEstimator estimator;
    estimator.init(0.5f);

    EstimatedState state = estimator.update(makeEstimatorInput(1000000), 100.0f, true);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, state.rollDeg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, state.pitchDeg);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, state.yawDeg);
}

void test_estimator_filters_altitude_and_computes_vertical_speed() {
    ComplementaryEstimator estimator;
    estimator.init(0.5f);

    estimator.update(makeEstimatorInput(1000000), 100.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(2000000), 110.0f, true);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 105.0f, state.altitudeM);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, state.verticalSpeedMps);
}

void test_estimator_rejects_invalid_sensor_health() {
    ComplementaryEstimator estimator;
    estimator.init(0.5f);

    EstimatedState state = estimator.update(makeEstimatorInput(1000000, SensorHealth::Stale), 100.0f, true);

    TEST_ASSERT_FALSE(state.valid);
    TEST_ASSERT_EQUAL_INT((int)SensorHealth::Stale, (int)state.health);
}

void test_estimator_handles_missing_baro() {
    ComplementaryEstimator estimator;
    estimator.init(0.5f);

    EstimatedState state = estimator.update(makeEstimatorInput(1000000), 0.0f, false);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.altitudeM);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.verticalSpeedMps);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_estimator_copies_attitude);
    RUN_TEST(test_estimator_filters_altitude_and_computes_vertical_speed);
    RUN_TEST(test_estimator_rejects_invalid_sensor_health);
    RUN_TEST(test_estimator_handles_missing_baro);
    return UNITY_END();
}
