#include <unity.h>

#include "estimators/EkfLiteEstimator.h"

#include "../../src/estimators/EkfLiteEstimator.cpp"

static EstimatorInput makeEstimatorInput(uint32_t timestamp, SensorHealth health = SensorHealth::Ok, bool failsafe = false) {
    EstimatorInput input = {};
    input.rollDeg = 4.0f;
    input.pitchDeg = -1.0f;
    input.yawDeg = 12.0f;
    input.timestampUs = timestamp;
    input.sensorHealth = health;
    input.failsafe = failsafe;
    return input;
}

void test_ekf_lite_initializes_from_first_baro_sample() {
    EkfLiteEstimator estimator;
    estimator.init();

    EstimatedState state = estimator.update(makeEstimatorInput(1000000), 120.0f, true);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 120.0f, state.altitudeM);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.verticalSpeedMps);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, state.rollDeg);
}

void test_ekf_lite_tracks_linear_climb_without_large_lag() {
    EkfLiteEstimator estimator;
    estimator.init({0.08f, 0.6f, 1.2f, 20.0f});

    estimator.update(makeEstimatorInput(1000000), 100.0f, true);
    estimator.update(makeEstimatorInput(2000000), 105.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(3000000), 110.0f, true);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_TRUE(state.altitudeM > 104.0f);
    TEST_ASSERT_TRUE(state.altitudeM < 111.0f);
    TEST_ASSERT_TRUE(state.verticalSpeedMps > 0.5f);
}

void test_ekf_lite_rejects_large_baro_spike() {
    EkfLiteEstimator estimator;
    estimator.init({0.08f, 0.35f, 1.0f, 8.0f});

    estimator.update(makeEstimatorInput(1000000), 50.0f, true);
    estimator.update(makeEstimatorInput(2000000), 51.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(3000000), 150.0f, true);

    TEST_ASSERT_TRUE(estimator.wasLastMeasurementRejected());
    TEST_ASSERT_TRUE(fabsf(estimator.getLastInnovationM()) > 8.0f);
    TEST_ASSERT_TRUE(state.altitudeM < 70.0f);
}

void test_ekf_lite_handles_missing_baro_by_prediction() {
    EkfLiteEstimator estimator;
    estimator.init({0.08f, 0.6f, 1.2f, 20.0f});

    estimator.update(makeEstimatorInput(1000000), 100.0f, true);
    estimator.update(makeEstimatorInput(2000000), 105.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(3000000), 0.0f, false);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_TRUE(state.altitudeM >= 100.0f);
}

void test_ekf_lite_marks_invalid_when_sensor_health_is_bad() {
    EkfLiteEstimator estimator;
    estimator.init();

    EstimatedState state = estimator.update(makeEstimatorInput(1000000, SensorHealth::Stale), 100.0f, true);

    TEST_ASSERT_FALSE(state.valid);
    TEST_ASSERT_EQUAL_INT((int)SensorHealth::Stale, (int)state.health);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ekf_lite_initializes_from_first_baro_sample);
    RUN_TEST(test_ekf_lite_tracks_linear_climb_without_large_lag);
    RUN_TEST(test_ekf_lite_rejects_large_baro_spike);
    RUN_TEST(test_ekf_lite_handles_missing_baro_by_prediction);
    RUN_TEST(test_ekf_lite_marks_invalid_when_sensor_health_is_bad);
    return UNITY_END();
}
