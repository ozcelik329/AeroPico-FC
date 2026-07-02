#include "ParamManager.h"
#include "../core/SystemTimer.h"

#ifdef MAVLINK_PARAMS_ENABLED

ParamManager paramManager;

void ParamManager::init() {
    Serial.println("[PARAMS] Parametre yoneticisi baslatildi.");
    Serial.printf("[PARAMS] %d parametre yuklu.\n", PARAM_COUNT);
}

void ParamManager::handleMessage(const mavlink_message_t& msg) {
    switch (msg.msgid) {
        case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
            _handleParamRequestList(msg);
            break;
        case MAVLINK_MSG_ID_PARAM_SET:
            _handleParamSet(msg);
            break;
        default:
            break;
    }
}

void ParamManager::sendAll() {
    for (uint8_t i = 0; i < PARAM_COUNT; i++) {
        sendParam(i);
        delay(10);  // GCS'in işlemesi için küçük gecikme
    }
}

void ParamManager::sendParam(uint8_t index) {
    if (index >= PARAM_COUNT) return;

    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    mavlink_msg_param_value_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        _params[index].name,
        _params[index].value,
        MAV_PARAM_TYPE_REAL32,
        PARAM_COUNT,
        index
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    espUart.write(buf, len);
}

void ParamManager::_handleParamRequestList(const mavlink_message_t& msg) {
    Serial.println("[PARAMS] GCS parametre listesi istedi.");
    sendAll();
}

void ParamManager::_handleParamSet(const mavlink_message_t& msg) {
    mavlink_param_set_t set;
    mavlink_msg_param_set_decode(&msg, &set);

    for (uint8_t i = 0; i < PARAM_COUNT; i++) {
        if (strncmp(_params[i].name, set.param_id, 16) == 0) {
            // Sınır kontrolü
            float newVal = constrain(set.param_value,
                                     _params[i].minVal,
                                     _params[i].maxVal);
            _params[i].value = newVal;

            Serial.printf("[PARAMS] %s = %.4f\n", _params[i].name, newVal);

            SystemTimer::applyPidGains(
                getAngleP(), getAngleI(), getAngleD(),
                getRateP(), getRateI(), getRateD()
            );

            // Onay olarak geri gönder
            sendParam(i);
            return;
        }
    }

    Serial.println("[PARAMS] Bilinmeyen parametre!");
}

void ParamManager::_sendPacket(mavlink_message_t& msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    espUart.write(buf, len);
}

#endif  // MAVLINK_PARAMS_ENABLED
