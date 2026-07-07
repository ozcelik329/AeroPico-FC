#ifndef PARAM_MANAGER_H
#define PARAM_MANAGER_H

// ESP32-CAM gelene kadar deaktif
#ifdef MAVLINK_PARAMS_ENABLED

#include <Arduino.h>
#include <common/mavlink.h>
#include "../drivers/PioUart.h"
#include "../config.h"
#include "../core/FixedWingMixer.h"

// Desteklenen parametreler
#define PARAM_COUNT 19

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

    void init();
    void setPidGainsApplyHandler(PidGainsApplyHandler handler);
    void setMixerSettingsApplyHandler(MixerSettingsApplyHandler handler);
    void setFailsafeTimeoutApplyHandler(FailsafeTimeoutApplyHandler handler);
    bool setParamByName(const char* name, float value);

    // Gelen MAVLink mesajını işle
    void handleMessage(const mavlink_message_t& msg);

    // Tüm parametreleri GCS'e gönder
    void sendAll();

    // Tek parametre gönder
    void sendParam(uint8_t index);

    // Değerleri config'e yaz
    float getAngleP() const { return _params[0].value; }
    float getAngleI() const { return _params[1].value; }
    float getAngleD() const { return _params[2].value; }
    float getRateP()  const { return _params[3].value; }
    float getRateI()  const { return _params[4].value; }
    float getRateD()  const { return _params[5].value; }
    MixerSettings getMixerSettings() const;
    uint32_t getFailsafeTimeoutMs() const { return (uint32_t)_params[18].value; }

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
    };
    PidGainsApplyHandler _pidGainsApplyHandler = nullptr;
    MixerSettingsApplyHandler _mixerSettingsApplyHandler = nullptr;
    FailsafeTimeoutApplyHandler _failsafeTimeoutApplyHandler = nullptr;

    bool _isPidParam(uint8_t index) const;
    bool _isMixerParam(uint8_t index) const;
    void _handleParamRequestList(const mavlink_message_t& msg);
    void _handleParamSet(const mavlink_message_t& msg);
    void _sendPacket(mavlink_message_t& msg);
    int _findParamIndex(const char* name) const;
};

extern ParamManager paramManager;

#endif  // MAVLINK_PARAMS_ENABLED
#endif  // PARAM_MANAGER_H
