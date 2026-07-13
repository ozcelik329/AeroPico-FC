#include <unity.h>

#include "core/safety/PreflightHealth.h"

#include "../../src/core/safety/PreflightHealth.cpp"

void test_preflight_allows_arm_when_required_checks_pass() {
    PreflightHealth health;
    health.reset();
    health.setCheck(PreflightCheckId::Boot, true, true, "");
    health.setCheck(PreflightCheckId::Sensor, true, true, "");
    health.setCheck(PreflightCheckId::RC, true, true, "");

    PreflightResult result = health.evaluate();

    TEST_ASSERT_TRUE(result.canArm);
    TEST_ASSERT_EQUAL_UINT8(0, result.failedRequiredCount);
}

void test_preflight_blocks_arm_with_reason() {
    PreflightHealth health;
    health.reset();
    health.setCheck(PreflightCheckId::Boot, true, true, "");
    health.setCheck(PreflightCheckId::Sensor, true, false, "IMU not healthy");
    health.setCheck(PreflightCheckId::RC, true, false, "RC missing");

    PreflightResult result = health.evaluate();

    TEST_ASSERT_FALSE(result.canArm);
    TEST_ASSERT_EQUAL_UINT8(2, result.failedRequiredCount);
    TEST_ASSERT_EQUAL_STRING("IMU not healthy", result.firstFailureReason);
}

void test_preflight_ignores_optional_failure_for_arm() {
    PreflightHealth health;
    health.reset();
    health.setCheck(PreflightCheckId::GPS, false, false, "GPS not configured");
    health.setCheck(PreflightCheckId::RC, true, true, "");

    PreflightResult result = health.evaluate();

    TEST_ASSERT_TRUE(result.canArm);
    TEST_ASSERT_EQUAL_UINT8(0, result.failedRequiredCount);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_preflight_allows_arm_when_required_checks_pass);
    RUN_TEST(test_preflight_blocks_arm_with_reason);
    RUN_TEST(test_preflight_ignores_optional_failure_for_arm);
    return UNITY_END();
}
