#ifndef PARAM_MANAGER_H
#define PARAM_MANAGER_H

// ESP32-CAM gelene kadar deaktif
#ifdef MAVLINK_PARAMS_ENABLED

#include <Arduino.h>
#include <common/mavlink.h>
#include "../drivers/PioUart.h"
#include "../config.h"
#include "../core/FixedWingMixer.h"
#include "../storage/ParamStorage.h"

// Desteklenen parametreler
#define PARAM_PERSISTED_COUNT 28
#define PARAM_COUNT 29

enum ParamIndex : uint8_t {
    PARAM_IDX_ANGLE_P = 0,
    PARAM_IDX_ANGLE_I,
    PARAM_IDX_ANGLE_D,
    PARAM_IDX_RATE_P,
    PARAM_IDX_RATE_I,
    PARAM_IDX_RATE_D,
    PARAM_IDX_MIX_ROLL,
    PARAM_IDX_MIX_PITCH,
    PARAM_IDX_MIX_YAW,
    PARAM_IDX_TRIM_AIL,
    PARAM_IDX_TRIM_ELE,
    PARAM_IDX_TRIM_RUD,
    PARAM_IDX_TRIM_THR,
    PARAM_IDX_REV_AIL,
    PARAM_IDX_REV_ELE,
    PARAM_IDX_REV_RUD,
    PARAM_IDX_SERVO_MIN,
    PARAM_IDX_SERVO_MAX,
    PARAM_IDX_FS_TIMEOUT,
    PARAM_IDX_RC_ROLL_CH,
    PARAM_IDX_RC_PITCH_CH,
    PARAM_IDX_RC_THR_CH,
    PARAM_IDX_RC_YAW_CH,
    PARAM_IDX_MAV_ATT_HZ,
    PARAM_IDX_MAV_RC_HZ,
    PARAM_IDX_MAV_SYS_HZ,
    PARAM_IDX_BB_LOG_HZ,
    PARAM_IDX_PREF_Q_MIN,
    PARAM_IDX_SAVE
};

struct Param {
    char     name[17];   // MAVLink param_id max 16 karakter
    float    value;
    float    minVal;
    float    maxVal;
};

class ParamManager {
  public:
    using PidGainsApplyHandler = void (*)(float angleP, float angleI, float angleD,
                                          float rateP, float rateI, float rateD);
    using MixerSettingsApplyHandler = void (*)(const MixerSettings& settings);
    using FailsafeTimeoutApplyHandler = void (*)(uint32_t timeoutMs);
    using RcMappingApplyHandler = void (*)(uint8_t roll, uint8_t pitch, uint8_t throttle, uint8_t yaw);
    using MavlinkRatesApplyHandler = void (*)(uint8_t attitudeHz, uint8_t rcHz, uint8_t sysStatusHz);
    using BlackboxRateApplyHandler = void (*)(uint8_t logHz);
    using PreflightQualityApplyHandler = void (*)(uint8_t minQuality);
    using ArmStateProvider = bool (*)();

    void init();
    void setPidGainsApplyHandler(PidGainsApplyHandler handler);
    void setMixerSettingsApplyHandler(MixerSettingsApplyHandler handler);
    void setFailsafeTimeoutApplyHandler(FailsafeTimeoutApplyHandler handler);
    void setRcMappingApplyHandler(RcMappingApplyHandler handler);
    void setMavlinkRatesApplyHandler(MavlinkRatesApplyHandler handler);
    void setBlackboxRateApplyHandler(BlackboxRateApplyHandler handler);
    void setPreflightQualityApplyHandler(PreflightQualityApplyHandler handler);
    void setArmStateProvider(ArmStateProvider provider);
    void setStorage(IParamStorage* storage);
    bool loadPersistent();
    bool savePersistent();
    bool setParamByName(const char* name, float value);
    bool isDirty() const { return _dirty; }
    const char* getLastError() const { return _lastError; }

    // Gelen MAVLink mesajını işle
    void handleMessage(const mavlink_message_t& msg);

    // Tüm parametreleri GCS'e gönder
    void sendAll();
    void processSendQueue(uint32_t nowMs);
    bool isSendActive() const { return _sendActive; }
    uint8_t pendingSendCount() const {
        return _sendActive ? (uint8_t)(PARAM_COUNT - _sendIndex) : 0;
    }

    // Tek parametre gönder
    void sendParam(uint8_t index);

    // Değerleri config'e yaz
    float getAngleP() const { return _params[PARAM_IDX_ANGLE_P].value; }
    float getAngleI() const { return _params[PARAM_IDX_ANGLE_I].value; }
    float getAngleD() const { return _params[PARAM_IDX_ANGLE_D].value; }
    float getRateP()  const { return _params[PARAM_IDX_RATE_P].value; }
    float getRateI()  const { return _params[PARAM_IDX_RATE_I].value; }
    float getRateD()  const { return _params[PARAM_IDX_RATE_D].value; }
    MixerSettings getMixerSettings() const;
    uint32_t getFailsafeTimeoutMs() const { return (uint32_t)_params[PARAM_IDX_FS_TIMEOUT].value; }
    uint8_t getRcRollChannel() const { return (uint8_t)_params[PARAM_IDX_RC_ROLL_CH].value; }
    uint8_t getRcPitchChannel() const { return (uint8_t)_params[PARAM_IDX_RC_PITCH_CH].value; }
    uint8_t getRcThrottleChannel() const { return (uint8_t)_params[PARAM_IDX_RC_THR_CH].value; }
    uint8_t getRcYawChannel() const { return (uint8_t)_params[PARAM_IDX_RC_YAW_CH].value; }
    uint8_t getMavlinkAttitudeHz() const { return (uint8_t)_params[PARAM_IDX_MAV_ATT_HZ].value; }
    uint8_t getMavlinkRcHz() const { return (uint8_t)_params[PARAM_IDX_MAV_RC_HZ].value; }
    uint8_t getMavlinkSysStatusHz() const { return (uint8_t)_params[PARAM_IDX_MAV_SYS_HZ].value; }
    uint8_t getBlackboxLogHz() const { return (uint8_t)_params[PARAM_IDX_BB_LOG_HZ].value; }
    uint8_t getPreflightMinQuality() const { return (uint8_t)_params[PARAM_IDX_PREF_Q_MIN].value; }

  private:
    Param _params[PARAM_COUNT] = {
        {"ANGLE_P", ANGLE_P_GAIN, 0.0f, 10.0f},
        {"ANGLE_I", ANGLE_I_GAIN, 0.0f,  1.0f},
        {"ANGLE_D", ANGLE_D_GAIN, 0.0f,  1.0f},
        {"RATE_P",  RATE_P_GAIN,  0.0f, 10.0f},
        {"RATE_I",  RATE_I_GAIN,  0.0f,  1.0f},
        {"RATE_D",  RATE_D_GAIN,  0.0f,  1.0f},
        {"MIX_ROLL", 1.0f, 0.0f, 2.0f},
        {"MIX_PITCH", 1.0f, 0.0f, 2.0f},
        {"MIX_YAW", 0.8f, 0.0f, 2.0f},
        {"TRIM_AIL", 0.0f, -300.0f, 300.0f},
        {"TRIM_ELE", 0.0f, -300.0f, 300.0f},
        {"TRIM_RUD", 0.0f, -300.0f, 300.0f},
        {"TRIM_THR", 0.0f, -300.0f, 300.0f},
        {"REV_AIL", 0.0f, 0.0f, 1.0f},
        {"REV_ELE", 0.0f, 0.0f, 1.0f},
        {"REV_RUD", 0.0f, 0.0f, 1.0f},
        {"SERVO_MIN", PWM_MIN, PWM_MIN, PWM_NEUTRAL},
        {"SERVO_MAX", PWM_MAX, PWM_NEUTRAL, PWM_MAX},
        {"FS_TIMEOUT", FAILSAFE_TIMEOUT_MS, 50.0f, 5000.0f},
        {"RC_ROLL_CH", RC_ROLL_CHANNEL, 0.0f, 7.0f},
        {"RC_PITCH_CH", RC_PITCH_CHANNEL, 0.0f, 7.0f},
        {"RC_THR_CH", RC_THROTTLE_CHANNEL, 0.0f, 7.0f},
        {"RC_YAW_CH", RC_YAW_CHANNEL, 0.0f, 7.0f},
        {"MAV_ATT_HZ", 10.0f, 1.0f, 50.0f},
        {"MAV_RC_HZ", 5.0f, 1.0f, 25.0f},
        {"MAV_SYS_HZ", 2.0f, 1.0f, 10.0f},
        {"BB_LOG_HZ", 5.0f, 1.0f, 50.0f},
        {"PREF_Q_MIN", 60.0f, 1.0f, 100.0f},
        {"PARAM_SAVE", 0.0f, 0.0f, 1.0f},
    };
    PidGainsApplyHandler _pidGainsApplyHandler = nullptr;
    MixerSettingsApplyHandler _mixerSettingsApplyHandler = nullptr;
    FailsafeTimeoutApplyHandler _failsafeTimeoutApplyHandler = nullptr;
    RcMappingApplyHandler _rcMappingApplyHandler = nullptr;
    MavlinkRatesApplyHandler _mavlinkRatesApplyHandler = nullptr;
    BlackboxRateApplyHandler _blackboxRateApplyHandler = nullptr;
    PreflightQualityApplyHandler _preflightQualityApplyHandler = nullptr;
    ArmStateProvider _armStateProvider = nullptr;
    IParamStorage* _storage = nullptr;
    bool _dirty = false;
    const char* _lastError = "";
    uint8_t _sendIndex = 0;
    uint32_t _nextSendMs = 0;
    bool _sendActive = false;

    bool _isPidParam(uint8_t index) const;
    bool _isMixerParam(uint8_t index) const;
    bool _isRcMappingParam(uint8_t index) const;
    bool _isMavlinkRateParam(uint8_t index) const;
    void _applyParam(uint8_t index);
    void _handleParamRequestList(const mavlink_message_t& msg);
    void _handleParamSet(const mavlink_message_t& msg);
    void _sendPacket(mavlink_message_t& msg);
    int _findParamIndex(const char* name) const;
};

extern ParamManager paramManager;

#endif  // MAVLINK_PARAMS_ENABLED
#endif  // PARAM_MANAGER_H
