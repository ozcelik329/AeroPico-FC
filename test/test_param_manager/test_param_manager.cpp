#include <unity.h>

#include "telemetry/ParamManager.h"

#include "../../src/storage/ParamStorage.cpp"
#include "../../src/telemetry/ParamManager.cpp"

PioUart espUart;

static bool gainsApplied;
static bool mixerApplied;
static bool failsafeApplied;
static bool rcMappingApplied;
static bool mavlinkRatesApplied;
static bool blackboxRateApplied;
static bool preflightQualityApplied;
static float appliedAngleP;
static float appliedRateD;
static MixerSettings appliedMixerSettings;
static uint32_t appliedFailsafeTimeoutMs;
static uint8_t appliedRcRoll;
static uint8_t appliedRcPitch;
static uint8_t appliedRcThrottle;
static uint8_t appliedRcYaw;
static uint8_t appliedMavAtt;
static uint8_t appliedMavRc;
static uint8_t appliedMavSys;
static uint8_t appliedBlackboxHz;
static uint8_t appliedPreflightQuality;

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

static void applyRcMapping(uint8_t roll, uint8_t pitch, uint8_t throttle, uint8_t yaw) {
    rcMappingApplied = true;
    appliedRcRoll = roll;
    appliedRcPitch = pitch;
    appliedRcThrottle = throttle;
    appliedRcYaw = yaw;
}

static void applyMavlinkRates(uint8_t attitudeHz, uint8_t rcHz, uint8_t sysStatusHz) {
    mavlinkRatesApplied = true;
    appliedMavAtt = attitudeHz;
    appliedMavRc = rcHz;
    appliedMavSys = sysStatusHz;
}

static void applyBlackboxRate(uint8_t logHz) {
    blackboxRateApplied = true;
    appliedBlackboxHz = logHz;
}

static void applyPreflightQuality(uint8_t minQuality) {
    preflightQualityApplied = true;
    appliedPreflightQuality = minQuality;
}

void setUp() {
    gainsApplied = false;
    mixerApplied = false;
    failsafeApplied = false;
    rcMappingApplied = false;
    mavlinkRatesApplied = false;
    blackboxRateApplied = false;
    preflightQualityApplied = false;
    appliedAngleP = 0.0f;
    appliedRateD = 0.0f;
    appliedFailsafeTimeoutMs = 0;
    appliedRcRoll = appliedRcPitch = appliedRcThrottle = appliedRcYaw = 0;
    appliedMavAtt = appliedMavRc = appliedMavSys = 0;
    appliedBlackboxHz = 0;
    appliedPreflightQuality = 0;
    paramManager = ParamManager();
    paramManager.setPidGainsApplyHandler(applyGains);
    paramManager.setMixerSettingsApplyHandler(applyMixer);
    paramManager.setFailsafeTimeoutApplyHandler(applyFailsafeTimeout);
    paramManager.setRcMappingApplyHandler(applyRcMapping);
    paramManager.setMavlinkRatesApplyHandler(applyMavlinkRates);
    paramManager.setBlackboxRateApplyHandler(applyBlackboxRate);
    paramManager.setPreflightQualityApplyHandler(applyPreflightQuality);
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

void test_param_manager_marks_dirty_and_saves_on_command() {
    MemoryParamStorage storage;
    paramManager.setStorage(&storage);

    TEST_ASSERT_TRUE(paramManager.setParamByName("ANGLE_P", 4.0f));
    TEST_ASSERT_TRUE(paramManager.isDirty());
    TEST_ASSERT_TRUE(paramManager.setParamByName("PARAM_SAVE", 1.0f));
    TEST_ASSERT_FALSE(paramManager.isDirty());

    ParamManager loaded;
    loaded.setStorage(&storage);
    TEST_ASSERT_TRUE(loaded.loadPersistent());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, loaded.getAngleP());
}

void test_param_manager_applies_rc_mapping_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("RC_ROLL_CH", 3.0f));
    TEST_ASSERT_TRUE(rcMappingApplied);
    TEST_ASSERT_EQUAL_UINT8(3, appliedRcRoll);
    TEST_ASSERT_EQUAL_UINT8(RC_PITCH_CHANNEL, appliedRcPitch);
}

void test_param_manager_applies_stream_and_log_callbacks() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("MAV_ATT_HZ", 20.0f));
    TEST_ASSERT_TRUE(mavlinkRatesApplied);
    TEST_ASSERT_EQUAL_UINT8(20, appliedMavAtt);

    TEST_ASSERT_TRUE(paramManager.setParamByName("BB_LOG_HZ", 12.0f));
    TEST_ASSERT_TRUE(blackboxRateApplied);
    TEST_ASSERT_EQUAL_UINT8(12, appliedBlackboxHz);
}

void test_param_manager_applies_preflight_quality_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("PREF_Q_MIN", 75.0f));
    TEST_ASSERT_TRUE(preflightQualityApplied);
    TEST_ASSERT_EQUAL_UINT8(75, appliedPreflightQuality);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_param_manager_sets_known_param_and_applies_callback);
    RUN_TEST(test_param_manager_clamps_param_to_range);
    RUN_TEST(test_param_manager_rejects_unknown_param);
    RUN_TEST(test_param_manager_applies_mixer_callback);
    RUN_TEST(test_param_manager_applies_failsafe_timeout_callback);
    RUN_TEST(test_param_manager_marks_dirty_and_saves_on_command);
    RUN_TEST(test_param_manager_applies_rc_mapping_callback);
    RUN_TEST(test_param_manager_applies_stream_and_log_callbacks);
    RUN_TEST(test_param_manager_applies_preflight_quality_callback);
    return UNITY_END();
}
