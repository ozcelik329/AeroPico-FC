#ifndef PARAM_MANAGER_H
#define PARAM_MANAGER_H

// ESP32-CAM gelene kadar deaktif
#ifdef MAVLINK_PARAMS_ENABLED

#include <Arduino.h>
#include <common/mavlink.h>
#include "../drivers/PioUart.h"
#include "../config.h"

// Desteklenen parametreler
#define PARAM_COUNT 6

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

    void init();
    void setPidGainsApplyHandler(PidGainsApplyHandler handler);
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

  private:
    Param _params[PARAM_COUNT] = {
        {"ANGLE_P", ANGLE_P_GAIN, 0.0f, 10.0f},
        {"ANGLE_I", ANGLE_I_GAIN, 0.0f,  1.0f},
        {"ANGLE_D", ANGLE_D_GAIN, 0.0f,  1.0f},
        {"RATE_P",  RATE_P_GAIN,  0.0f, 10.0f},
        {"RATE_I",  RATE_I_GAIN,  0.0f,  1.0f},
        {"RATE_D",  RATE_D_GAIN,  0.0f,  1.0f},
    };
    PidGainsApplyHandler _pidGainsApplyHandler = nullptr;

    void _handleParamRequestList(const mavlink_message_t& msg);
    void _handleParamSet(const mavlink_message_t& msg);
    void _sendPacket(mavlink_message_t& msg);
    int _findParamIndex(const char* name) const;
};

extern ParamManager paramManager;

#endif  // MAVLINK_PARAMS_ENABLED
#endif  // PARAM_MANAGER_H
