#include <unity.h>

#include "drivers/sensors/SensorHealthMonitor.h"

#include "../../src/drivers/sensors/SensorHealthMonitor.cpp"

void test_sensor_health_monitor_reports_invalid_without_imu() {
    SensorHealthMonitor monitor;

    SensorHealth health = monitor.evaluate(false, true, 1000, 1100, 5000);

    TEST_ASSERT_EQUAL((int)SensorHealth::Invalid, (int)health);
}

void test_sensor_health_monitor_reports_warming_up_without_sample() {
    SensorHealthMonitor monitor;

    SensorHealth health = monitor.evaluate(true, false, 0, 1100, 5000);

    TEST_ASSERT_EQUAL((int)SensorHealth::WarmingUp, (int)health);
}

void test_sensor_health_monitor_reports_stale_sample() {
    SensorHealthMonitor monitor;

    SensorHealth health = monitor.evaluate(true, true, 1000, 9000, 5000);

    TEST_ASSERT_EQUAL((int)SensorHealth::Stale, (int)health);
}

void test_sensor_health_monitor_reports_ok_sample() {
    SensorHealthMonitor monitor;

    SensorHealth health = monitor.evaluate(true, true, 1000, 2000, 5000);

    TEST_ASSERT_EQUAL((int)SensorHealth::Ok, (int)health);
}

void test_sensor_health_monitor_reports_quality_score_and_age() {
    SensorHealthMonitor monitor;

    SensorQuality quality = monitor.evaluateQuality(true, true, 1000, 2000, 5000);

    TEST_ASSERT_EQUAL((int)SensorHealth::Ok, (int)quality.health);
    TEST_ASSERT_EQUAL_UINT32(1000, quality.ageUs);
    TEST_ASSERT_TRUE(quality.score >= 50);
    TEST_ASSERT_TRUE(quality.score <= 100);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_health_monitor_reports_invalid_without_imu);
    RUN_TEST(test_sensor_health_monitor_reports_warming_up_without_sample);
    RUN_TEST(test_sensor_health_monitor_reports_stale_sample);
    RUN_TEST(test_sensor_health_monitor_reports_ok_sample);
    RUN_TEST(test_sensor_health_monitor_reports_quality_score_and_age);
    return UNITY_END();
}
