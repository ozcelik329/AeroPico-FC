#include <math.h>
#include <unity.h>

#include "core/SensorFusion.h"

#include "../../src/core/SensorFusion.cpp"

void test_sensor_fusion_starts_level() {
    SensorFusion fusion;
    fusion.init(0.1f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fusion.getRoll());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fusion.getPitch());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fusion.getYaw());
}

void test_sensor_fusion_known_roll_quaternion() {
    SensorFusion fusion;
    fusion.init(0.1f);

    const float half = 45.0f * DEG_TO_RAD;
    fusion.setQuaternionForTest(cosf(half), sinf(half), 0.0f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, fusion.getRoll());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, fusion.getPitch());
}

void test_sensor_fusion_known_pitch_quaternion() {
    SensorFusion fusion;
    fusion.init(0.1f);

    const float half = 15.0f * DEG_TO_RAD;
    fusion.setQuaternionForTest(cosf(half), 0.0f, sinf(half), 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, fusion.getPitch());
}

void test_sensor_fusion_rejects_zero_accel_update() {
    SensorFusion fusion;
    fusion.init(0.1f);
    setMockMicros(1000);
    fusion.updateIMU(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fusion.getRoll());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fusion.getPitch());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_fusion_starts_level);
    RUN_TEST(test_sensor_fusion_known_roll_quaternion);
    RUN_TEST(test_sensor_fusion_known_pitch_quaternion);
    RUN_TEST(test_sensor_fusion_rejects_zero_accel_update);
    return UNITY_END();
}
