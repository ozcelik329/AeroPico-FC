#include <unity.h>

#include "telemetry/ParamManager.h"

#include "../../src/telemetry/ParamManager.cpp"

PioUart espUart;

static bool gainsApplied;
static float appliedAngleP;
static float appliedRateD;

static void applyGains(float angleP, float angleI, float angleD,
                       float rateP, float rateI, float rateD) {
    (void)angleI;
    (void)angleD;
    (void)rateP;
    (void)rateI;
    gainsApplied = true;
    appliedAngleP = angleP;
    appliedRateD = rateD;
}

void setUp() {
    gainsApplied = false;
    appliedAngleP = 0.0f;
    appliedRateD = 0.0f;
    paramManager = ParamManager();
    paramManager.setPidGainsApplyHandler(applyGains);
}

void test_param_manager_sets_known_param_and_applies_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("ANGLE_P", 3.5f));
    TEST_ASSERT_TRUE(gainsApplied);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, paramManager.getAngleP());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, appliedAngleP);
}

void test_param_manager_clamps_param_to_range() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("RATE_D", 5.0f));
    TEST_ASSERT_TRUE(gainsApplied);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, paramManager.getRateD());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, appliedRateD);
}

void test_param_manager_rejects_unknown_param() {
    TEST_ASSERT_FALSE(paramManager.setParamByName("NOPE", 1.0f));
    TEST_ASSERT_FALSE(gainsApplied);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_param_manager_sets_known_param_and_applies_callback);
    RUN_TEST(test_param_manager_clamps_param_to_range);
    RUN_TEST(test_param_manager_rejects_unknown_param);
    return UNITY_END();
}
