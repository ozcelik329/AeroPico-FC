#include <unity.h>

#include "estimators/BaroVerticalKalman.h"

#include "../../src/estimators/BaroVerticalKalman.cpp"

static EstimatorInput makeEstimatorInput(uint32_t timestamp, SensorHealth health = SensorHealth::Ok, bool failsafe = false) {
    EstimatorInput input = {};
    input.rollDeg = 4.0f;
    input.pitchDeg = -1.0f;
    input.yawDeg = 12.0f;
    input.verticalAccelMps2 = 0.0f;
    input.timestampUs = timestamp;
    input.sensorHealth = health;
    input.failsafe = failsafe;
    return input;
}

static EstimatorInput makeEstimatorInputWithAccel(uint32_t timestamp, float verticalAccelMps2) {
    EstimatorInput input = makeEstimatorInput(timestamp);
    input.verticalAccelMps2 = verticalAccelMps2;
    return input;
}

void test_baro_vertical_kalman_initializes_from_first_baro_sample() {
    BaroVerticalKalman estimator;
    estimator.init();

    EstimatedState state = estimator.update(makeEstimatorInput(1000000), 120.0f, true);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 120.0f, state.altitudeM);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, state.verticalSpeedMps);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, state.rollDeg);
}

void test_baro_vertical_kalman_tracks_linear_climb_without_large_lag() {
    BaroVerticalKalman estimator;
    estimator.init({0.08f, 0.6f, 1.2f, 20.0f});

    estimator.update(makeEstimatorInput(1000000), 100.0f, true);
    estimator.update(makeEstimatorInput(2000000), 105.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(3000000), 110.0f, true);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_TRUE(state.altitudeM > 104.0f);
    TEST_ASSERT_TRUE(state.altitudeM < 111.0f);
    TEST_ASSERT_TRUE(state.verticalSpeedMps > 0.5f);
}

void test_baro_vertical_kalman_uses_vertical_acceleration_in_prediction() {
    BaroVerticalKalman estimator;
    estimator.init({0.08f, 0.6f, 1.2f, 20.0f, 1.0f});

    estimator.update(makeEstimatorInput(1000000), 100.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInputWithAccel(2000000, 2.0f), 0.0f, false);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_TRUE(state.verticalSpeedMps > 1.5f);
    TEST_ASSERT_TRUE(state.altitudeM > 100.5f);
}

void test_baro_vertical_kalman_rejects_large_baro_spike() {
    BaroVerticalKalman estimator;
    estimator.init({0.08f, 0.35f, 1.0f, 8.0f});

    estimator.update(makeEstimatorInput(1000000), 50.0f, true);
    estimator.update(makeEstimatorInput(2000000), 51.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(3000000), 150.0f, true);

    TEST_ASSERT_TRUE(estimator.wasLastMeasurementRejected());
    TEST_ASSERT_TRUE(fabsf(estimator.getLastInnovationM()) > 8.0f);
    TEST_ASSERT_TRUE(state.altitudeM < 70.0f);
}

void test_baro_vertical_kalman_marks_stale_after_repeated_rejections() {
    BaroVerticalKalman estimator;
    estimator.init({0.08f, 0.35f, 1.0f, 8.0f});

    estimator.update(makeEstimatorInput(1000000), 50.0f, true);
    estimator.update(makeEstimatorInput(2000000), 51.0f, true);
    estimator.update(makeEstimatorInput(3000000), 150.0f, true);
    estimator.update(makeEstimatorInput(4000000), 151.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(5000000), 152.0f, true);

    TEST_ASSERT_FALSE(state.valid);
    TEST_ASSERT_EQUAL_UINT8(3, estimator.consecutiveRejects());
    TEST_ASSERT_EQUAL_INT((int)SensorHealth::Stale, (int)state.health);
}

void test_baro_vertical_kalman_exposes_covariance_bounds() {
    BaroVerticalKalman estimator;
    estimator.init({0.08f, 0.35f, 1.0f, 8.0f});

    estimator.update(makeEstimatorInput(1000000), 50.0f, true);
    estimator.update(makeEstimatorInput(2000000), 51.0f, true);

    TEST_ASSERT_TRUE(estimator.getAltitudeVariance() > 0.0f);
    TEST_ASSERT_TRUE(estimator.getVelocityVariance() > 0.0f);
    TEST_ASSERT_TRUE(estimator.getAltitudeVariance() < 100000.1f);
    TEST_ASSERT_TRUE(estimator.getVelocityVariance() < 100000.1f);
}

void test_baro_vertical_kalman_handles_missing_baro_by_prediction() {
    BaroVerticalKalman estimator;
    estimator.init({0.08f, 0.6f, 1.2f, 20.0f});

    estimator.update(makeEstimatorInput(1000000), 100.0f, true);
    estimator.update(makeEstimatorInput(2000000), 105.0f, true);
    EstimatedState state = estimator.update(makeEstimatorInput(3000000), 0.0f, false);

    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_TRUE(state.altitudeM >= 100.0f);
}

void test_baro_vertical_kalman_marks_invalid_when_sensor_health_is_bad() {
    BaroVerticalKalman estimator;
    estimator.init();

    EstimatedState state = estimator.update(makeEstimatorInput(1000000, SensorHealth::Stale), 100.0f, true);

    TEST_ASSERT_FALSE(state.valid);
    TEST_ASSERT_EQUAL_INT((int)SensorHealth::Stale, (int)state.health);
}

void test_baro_vertical_kalman_rejects_nan_baro_sample() {
    BaroVerticalKalman estimator;
    estimator.init();

    EstimatedState state = estimator.update(makeEstimatorInput(1000000), NAN, true);

    TEST_ASSERT_FALSE(state.valid);
    TEST_ASSERT_FALSE(estimator.getState().valid);
}

void test_baro_vertical_kalman_rejects_nan_attitude() {
    BaroVerticalKalman estimator;
    estimator.init();
    EstimatorInput input = makeEstimatorInput(1000000);
    input.rollDeg = NAN;

    EstimatedState state = estimator.update(input, 100.0f, true);

    TEST_ASSERT_FALSE(state.valid);
    TEST_ASSERT_EQUAL_INT((int)SensorHealth::Invalid, (int)state.health);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_baro_vertical_kalman_initializes_from_first_baro_sample);
    RUN_TEST(test_baro_vertical_kalman_tracks_linear_climb_without_large_lag);
    RUN_TEST(test_baro_vertical_kalman_uses_vertical_acceleration_in_prediction);
    RUN_TEST(test_baro_vertical_kalman_rejects_large_baro_spike);
    RUN_TEST(test_baro_vertical_kalman_marks_stale_after_repeated_rejections);
    RUN_TEST(test_baro_vertical_kalman_exposes_covariance_bounds);
    RUN_TEST(test_baro_vertical_kalman_handles_missing_baro_by_prediction);
    RUN_TEST(test_baro_vertical_kalman_marks_invalid_when_sensor_health_is_bad);
    RUN_TEST(test_baro_vertical_kalman_rejects_nan_baro_sample);
    RUN_TEST(test_baro_vertical_kalman_rejects_nan_attitude);
    return UNITY_END();
}
