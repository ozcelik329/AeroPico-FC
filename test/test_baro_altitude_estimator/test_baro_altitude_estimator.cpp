#include <unity.h>

#include "estimators/BaroAltitudeEstimator.h"

#include "../../src/estimators/BaroAltitudeEstimator.cpp"

void test_baro_altitude_estimator_sets_home_on_first_valid_pressure() {
    BaroAltitudeEstimator estimator;
    estimator.init();
    float altitude = -1.0f;

    TEST_ASSERT_TRUE(estimator.update(1010.0f, true, altitude));
    TEST_ASSERT_TRUE(estimator.hasHome());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1010.0f, estimator.getHomePressureHpa());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, altitude);
}

void test_baro_altitude_estimator_uses_fast_relative_pressure_model() {
    BaroAltitudeEstimator estimator;
    estimator.init(8.43f);
    estimator.setHomePressure(1013.25f);
    float altitude = 0.0f;

    TEST_ASSERT_TRUE(estimator.update(1012.25f, true, altitude));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.43f, altitude);
}

void test_baro_altitude_estimator_rejects_invalid_pressure() {
    BaroAltitudeEstimator estimator;
    estimator.init();
    float altitude = 123.0f;

    TEST_ASSERT_FALSE(estimator.update(0.0f, true, altitude));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, altitude);
    TEST_ASSERT_FALSE(estimator.update(1010.0f, false, altitude));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_baro_altitude_estimator_sets_home_on_first_valid_pressure);
    RUN_TEST(test_baro_altitude_estimator_uses_fast_relative_pressure_model);
    RUN_TEST(test_baro_altitude_estimator_rejects_invalid_pressure);
    return UNITY_END();
}
