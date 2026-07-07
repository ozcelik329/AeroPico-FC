#include <unity.h>

#include "telemetry/ParamManager.h"

#include "../../src/telemetry/ParamManager.cpp"

PioUart espUart;

static bool gainsApplied;
static bool mixerApplied;
static bool failsafeApplied;
static float appliedAngleP;
static float appliedRateD;
static MixerSettings appliedMixerSettings;
static uint32_t appliedFailsafeTimeoutMs;

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

static void applyMixer(const MixerSettings& settings) {
    mixerApplied = true;
    appliedMixerSettings = settings;
}

static void applyFailsafeTimeout(uint32_t timeoutMs) {
    failsafeApplied = true;
    appliedFailsafeTimeoutMs = timeoutMs;
}

void setUp() {
    gainsApplied = false;
    mixerApplied = false;
    failsafeApplied = false;
    appliedAngleP = 0.0f;
    appliedRateD = 0.0f;
    appliedFailsafeTimeoutMs = 0;
    paramManager = ParamManager();
    paramManager.setPidGainsApplyHandler(applyGains);
    paramManager.setMixerSettingsApplyHandler(applyMixer);
    paramManager.setFailsafeTimeoutApplyHandler(applyFailsafeTimeout);
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

void test_param_manager_applies_mixer_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("REV_AIL", 1.0f));
    TEST_ASSERT_TRUE(mixerApplied);
    TEST_ASSERT_TRUE(appliedMixerSettings.reverseAileron);
}

void test_param_manager_applies_failsafe_timeout_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("FS_TIMEOUT", 9000.0f));
    TEST_ASSERT_TRUE(failsafeApplied);
    TEST_ASSERT_EQUAL_UINT32(5000, appliedFailsafeTimeoutMs);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_param_manager_sets_known_param_and_applies_callback);
    RUN_TEST(test_param_manager_clamps_param_to_range);
    RUN_TEST(test_param_manager_rejects_unknown_param);
    RUN_TEST(test_param_manager_applies_mixer_callback);
    RUN_TEST(test_param_manager_applies_failsafe_timeout_callback);
    return UNITY_END();
}
