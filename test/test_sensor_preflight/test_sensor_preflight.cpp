#include <unity.h>
#include <string.h>

#include "core/sensors/SensorPreflightEvaluator.h"

#include "../../src/core/sensors/SensorPreflightEvaluator.cpp"

static SensorBuffer healthySample() {
    SensorBuffer sample = {};
    sample.valid = true;
    sample.health = SensorHealth::Ok;
    sample.qualityScore = 90;
    sample.sampleAgeUs = 1200;
    return sample;
}

void test_sensor_preflight_rejects_missing_imu() {
    SensorPreflightStatus status = SensorPreflightEvaluator::evaluate(false, healthySample(), 60);

    TEST_ASSERT_FALSE(status.passed);
    TEST_ASSERT_EQUAL((int)SensorPreflightReason::ImuUnavailable, (int)status.reason);
}

void test_sensor_preflight_rejects_bad_health() {
    SensorBuffer sample = healthySample();
    sample.health = SensorHealth::Stale;

    SensorPreflightStatus status = SensorPreflightEvaluator::evaluate(true, sample, 60);

    TEST_ASSERT_FALSE(status.passed);
    TEST_ASSERT_EQUAL((int)SensorPreflightReason::HealthNotOk, (int)status.reason);
    TEST_ASSERT_EQUAL_UINT32(1200, status.sampleAgeUs);
}

void test_sensor_preflight_rejects_low_quality() {
    SensorBuffer sample = healthySample();
    sample.qualityScore = 45;

    SensorPreflightStatus status = SensorPreflightEvaluator::evaluate(true, sample, 60);

    TEST_ASSERT_FALSE(status.passed);
    TEST_ASSERT_EQUAL((int)SensorPreflightReason::QualityLow, (int)status.reason);
}

void test_sensor_preflight_accepts_healthy_sample_and_formats_reason() {
    SensorPreflightStatus status = SensorPreflightEvaluator::evaluate(true, healthySample(), 60);
    char reason[72] = {};
    SensorPreflightEvaluator::formatReason(status, reason, sizeof(reason));

    TEST_ASSERT_TRUE(status.passed);
    TEST_ASSERT_EQUAL((int)SensorPreflightReason::Ok, (int)status.reason);
    TEST_ASSERT_NOT_NULL(strstr(reason, "q=90"));
    TEST_ASSERT_NOT_NULL(strstr(reason, "age=1200"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_preflight_rejects_missing_imu);
    RUN_TEST(test_sensor_preflight_rejects_bad_health);
    RUN_TEST(test_sensor_preflight_rejects_low_quality);
    RUN_TEST(test_sensor_preflight_accepts_healthy_sample_and_formats_reason);
    return UNITY_END();
}
