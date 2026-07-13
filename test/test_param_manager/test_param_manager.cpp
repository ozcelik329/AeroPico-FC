#include <unity.h>

#include "telemetry/ParamManager.h"

#include "../../src/storage/ParamStorage.cpp"
#include "../../src/telemetry/MavlinkTransport.cpp"
#include "../../src/telemetry/ParamManager.cpp"

PioUart espUart;

static bool gainsApplied;
static bool mixerApplied;
static bool failsafeApplied;
static bool rcMappingApplied;
static bool mavlinkRatesApplied;
static bool blackboxRateApplied;
static bool preflightQualityApplied;
static bool batteryProfileApplied;
static float appliedAngleP;
static float appliedRateD;
static MixerSettings appliedMixerSettings;
static uint32_t appliedFailsafeTimeoutMs;
static uint8_t appliedRcRoll;
static uint8_t appliedRcPitch;
static uint8_t appliedRcThrottle;
static uint8_t appliedRcYaw;
static uint8_t appliedRcMode;
static uint8_t appliedMavAtt;
static uint8_t appliedMavRc;
static uint8_t appliedMavSys;
static uint8_t appliedBlackboxHz;
static uint8_t appliedPreflightQuality;
static uint8_t appliedBatteryCells;
static uint16_t appliedBatteryCapacityMah;
static uint8_t appliedBatteryCRating;
static float appliedBatteryNominalVoltage;
static float appliedBatteryLowVoltage;
static float appliedBatteryBrownoutVoltage;
static float appliedBatteryMaxVoltage;
static bool armed;

static bool provideArmState() {
    return armed;
}

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

static void applyRcMapping(uint8_t roll, uint8_t pitch, uint8_t throttle, uint8_t yaw, uint8_t mode) {
    rcMappingApplied = true;
    appliedRcRoll = roll;
    appliedRcPitch = pitch;
    appliedRcThrottle = throttle;
    appliedRcYaw = yaw;
    appliedRcMode = mode;
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

static void applyBatteryProfile(uint8_t cells,
                                float nominalVoltage,
                                uint16_t capacityMah,
                                uint8_t cRating,
                                float lowVoltage,
                                float brownoutVoltage,
                                float maxVoltage) {
    batteryProfileApplied = true;
    appliedBatteryCells = cells;
    appliedBatteryNominalVoltage = nominalVoltage;
    appliedBatteryCapacityMah = capacityMah;
    appliedBatteryCRating = cRating;
    appliedBatteryLowVoltage = lowVoltage;
    appliedBatteryBrownoutVoltage = brownoutVoltage;
    appliedBatteryMaxVoltage = maxVoltage;
}

void setUp() {
    gainsApplied = false;
    mixerApplied = false;
    failsafeApplied = false;
    rcMappingApplied = false;
    mavlinkRatesApplied = false;
    blackboxRateApplied = false;
    preflightQualityApplied = false;
    batteryProfileApplied = false;
    appliedAngleP = 0.0f;
    appliedRateD = 0.0f;
    appliedFailsafeTimeoutMs = 0;
    appliedRcRoll = appliedRcPitch = appliedRcThrottle = appliedRcYaw = appliedRcMode = 0;
    appliedMavAtt = appliedMavRc = appliedMavSys = 0;
    appliedBlackboxHz = 0;
    appliedPreflightQuality = 0;
    appliedBatteryCells = 0;
    appliedBatteryCapacityMah = 0;
    appliedBatteryCRating = 0;
    appliedBatteryNominalVoltage = 0.0f;
    appliedBatteryLowVoltage = 0.0f;
    appliedBatteryBrownoutVoltage = 0.0f;
    appliedBatteryMaxVoltage = 0.0f;
    armed = false;
    paramManager = ParamManager();
    paramManager.setPidGainsApplyHandler(applyGains);
    paramManager.setMixerSettingsApplyHandler(applyMixer);
    paramManager.setFailsafeTimeoutApplyHandler(applyFailsafeTimeout);
    paramManager.setRcMappingApplyHandler(applyRcMapping);
    paramManager.setMavlinkRatesApplyHandler(applyMavlinkRates);
    paramManager.setBlackboxRateApplyHandler(applyBlackboxRate);
    paramManager.setPreflightQualityApplyHandler(applyPreflightQuality);
    paramManager.setBatteryProfileApplyHandler(applyBatteryProfile);
    paramManager.setArmStateProvider(provideArmState);
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

void test_param_manager_loads_older_blob_and_keeps_new_defaults() {
    MemoryParamStorage storage;
    float values[PARAM_PERSISTED_COUNT] = {};
    for (uint8_t i = 0; i < PARAM_PERSISTED_COUNT; i++) {
        values[i] = 1.0f;
    }
    values[PARAM_IDX_MAV_ATT_HZ] = 22.0f;
    values[PARAM_IDX_PREF_Q_MIN] = 73.0f;

    ParamStorageBlob olderBlob = ParamStorage::makeBlob(values, PARAM_IDX_RC_MODE_CH);
    TEST_ASSERT_TRUE(storage.save(olderBlob));
    paramManager.setStorage(&storage);

    TEST_ASSERT_TRUE(paramManager.loadPersistent());
    TEST_ASSERT_TRUE(paramManager.isDirty());
    TEST_ASSERT_EQUAL_UINT8(22, paramManager.getMavlinkAttitudeHz());
    TEST_ASSERT_EQUAL_UINT8(73, paramManager.getPreflightMinQuality());
    TEST_ASSERT_EQUAL_UINT8(RC_MODE_CHANNEL, paramManager.getRcModeChannel());
    TEST_ASSERT_EQUAL_UINT8(BATTERY_CELL_COUNT, paramManager.getBatteryCellCount());
    TEST_ASSERT_EQUAL_UINT16(BATTERY_CAPACITY_MAH, paramManager.getBatteryCapacityMah());
}

void test_param_manager_rejects_save_while_armed() {
    MemoryParamStorage storage;
    paramManager.setStorage(&storage);
    TEST_ASSERT_TRUE(paramManager.setParamByName("ANGLE_P", 4.0f));

    armed = true;
    TEST_ASSERT_FALSE(paramManager.setParamByName("PARAM_SAVE", 1.0f));
    TEST_ASSERT_TRUE(paramManager.isDirty());
    TEST_ASSERT_EQUAL_STRING("Parameter save rejected while armed", paramManager.getLastError());

    ParamStorageBlob blob = {};
    TEST_ASSERT_FALSE(storage.load(blob));
}

void test_param_manager_rejects_runtime_change_while_armed() {
    armed = true;

    TEST_ASSERT_FALSE(paramManager.setParamByName("ANGLE_P", 4.0f));
    TEST_ASSERT_FALSE(gainsApplied);
    TEST_ASSERT_EQUAL_STRING("Parameter change rejected while armed", paramManager.getLastError());
}

void test_param_manager_applies_rc_mapping_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("RC_ROLL_CH", 3.0f));
    TEST_ASSERT_TRUE(rcMappingApplied);
    TEST_ASSERT_EQUAL_UINT8(3, appliedRcRoll);
    TEST_ASSERT_EQUAL_UINT8(RC_PITCH_CHANNEL, appliedRcPitch);
    TEST_ASSERT_EQUAL_UINT8(RC_MODE_CHANNEL, appliedRcMode);

    TEST_ASSERT_TRUE(paramManager.setParamByName("RC_MODE_CH", 5.0f));
    TEST_ASSERT_EQUAL_UINT8(5, appliedRcMode);
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

void test_param_manager_applies_battery_profile_callback() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("BATT_CELLS", 4.0f));
    TEST_ASSERT_TRUE(batteryProfileApplied);
    TEST_ASSERT_EQUAL_UINT8(4, appliedBatteryCells);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 17.08f, appliedBatteryMaxVoltage);

    TEST_ASSERT_TRUE(paramManager.setParamByName("BATT_CAP_MAH", 5000.0f));
    TEST_ASSERT_EQUAL_UINT16(5000, appliedBatteryCapacityMah);

    TEST_ASSERT_TRUE(paramManager.setParamByName("BATT_C_RATE", 60.0f));
    TEST_ASSERT_EQUAL_UINT8(60, appliedBatteryCRating);
}

void test_param_manager_clamps_battery_profile() {
    TEST_ASSERT_TRUE(paramManager.setParamByName("BATT_CELLS", 12.0f));
    TEST_ASSERT_EQUAL_UINT8(6, paramManager.getBatteryCellCount());
    TEST_ASSERT_TRUE(paramManager.setParamByName("BATT_BRN_V", 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, paramManager.getBatteryBrownoutVoltage());
}

void test_param_manager_schedules_non_blocking_parameter_stream() {
    paramManager.sendAll();
    TEST_ASSERT_TRUE(paramManager.isSendActive());
    TEST_ASSERT_EQUAL_UINT8(PARAM_COUNT, paramManager.pendingSendCount());

    paramManager.processSendQueue(100);
    TEST_ASSERT_TRUE(paramManager.isSendActive());
    TEST_ASSERT_EQUAL_UINT8(PARAM_COUNT - 1, paramManager.pendingSendCount());

    paramManager.processSendQueue(110);
    TEST_ASSERT_EQUAL_UINT8(PARAM_COUNT - 1, paramManager.pendingSendCount());
    paramManager.processSendQueue(120);
    TEST_ASSERT_EQUAL_UINT8(PARAM_COUNT - 2, paramManager.pendingSendCount());
}

void test_param_manager_rejects_message_for_other_system() {
    mavlink_message_t message;
    mavlink_msg_param_set_pack(
        42, 1, &message,
        77, MAV_COMPONENT_ID,
        "ANGLE_P", 7.0f, MAV_PARAM_TYPE_REAL32
    );
    paramManager.handleMessage(message);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, ANGLE_P_GAIN, paramManager.getAngleP());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_param_manager_sets_known_param_and_applies_callback);
    RUN_TEST(test_param_manager_clamps_param_to_range);
    RUN_TEST(test_param_manager_rejects_unknown_param);
    RUN_TEST(test_param_manager_applies_mixer_callback);
    RUN_TEST(test_param_manager_applies_failsafe_timeout_callback);
    RUN_TEST(test_param_manager_marks_dirty_and_saves_on_command);
    RUN_TEST(test_param_manager_loads_older_blob_and_keeps_new_defaults);
    RUN_TEST(test_param_manager_rejects_save_while_armed);
    RUN_TEST(test_param_manager_rejects_runtime_change_while_armed);
    RUN_TEST(test_param_manager_applies_rc_mapping_callback);
    RUN_TEST(test_param_manager_applies_stream_and_log_callbacks);
    RUN_TEST(test_param_manager_applies_preflight_quality_callback);
    RUN_TEST(test_param_manager_applies_battery_profile_callback);
    RUN_TEST(test_param_manager_clamps_battery_profile);
    RUN_TEST(test_param_manager_schedules_non_blocking_parameter_stream);
    RUN_TEST(test_param_manager_rejects_message_for_other_system);
    return UNITY_END();
}
