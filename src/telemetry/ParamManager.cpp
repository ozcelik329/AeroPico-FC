#include "ParamManager.h"
#include "MavlinkHandler.h"

#ifdef MAVLINK_PARAMS_ENABLED

ParamManager paramManager;

void ParamManager::init() {
    Serial.println("[PARAMS] Parametre yoneticisi baslatildi.");
    Serial.printf("[PARAMS] %d parametre yuklu.\n", PARAM_COUNT);
}

void ParamManager::setPidGainsApplyHandler(PidGainsApplyHandler handler) {
    _pidGainsApplyHandler = handler;
}

void ParamManager::setMixerSettingsApplyHandler(MixerSettingsApplyHandler handler) {
    _mixerSettingsApplyHandler = handler;
}

void ParamManager::setFailsafeTimeoutApplyHandler(FailsafeTimeoutApplyHandler handler) {
    _failsafeTimeoutApplyHandler = handler;
}

bool ParamManager::_isPidParam(uint8_t index) const {
    return index <= 5;
}

bool ParamManager::_isMixerParam(uint8_t index) const {
    return index >= 6 && index <= 17;
}

int ParamManager::_findParamIndex(const char* name) const {
    for (uint8_t i = 0; i < PARAM_COUNT; i++) {
        if (strncmp(_params[i].name, name, 16) == 0) {
            return i;
        }
    }
    return -1;
}

bool ParamManager::setParamByName(const char* name, float value) {
    int index = _findParamIndex(name);
    if (index < 0) return false;

    _params[index].value = constrain(value, _params[index].minVal, _params[index].maxVal);
    if (_isPidParam((uint8_t)index) && _pidGainsApplyHandler) {
        _pidGainsApplyHandler(
            getAngleP(), getAngleI(), getAngleD(),
            getRateP(), getRateI(), getRateD()
        );
    }
    if (_isMixerParam((uint8_t)index) && _mixerSettingsApplyHandler) {
        _mixerSettingsApplyHandler(getMixerSettings());
    }
    if (index == 18 && _failsafeTimeoutApplyHandler) {
        _failsafeTimeoutApplyHandler(getFailsafeTimeoutMs());
    }
    return true;
}

MixerSettings ParamManager::getMixerSettings() const {
    MixerSettings settings;
    settings.rollGain = _params[6].value;
    settings.pitchGain = _params[7].value;
    settings.yawGain = _params[8].value;
    settings.aileronTrim = (int)_params[9].value;
    settings.elevatorTrim = (int)_params[10].value;
    settings.rudderTrim = (int)_params[11].value;
    settings.throttleTrim = (int)_params[12].value;
    settings.reverseAileron = _params[13].value >= 0.5f;
    settings.reverseElevator = _params[14].value >= 0.5f;
    settings.reverseRudder = _params[15].value >= 0.5f;
    settings.servoMin = (int)_params[16].value;
    settings.servoMax = (int)_params[17].value;
    return settings;
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

    int index = _findParamIndex(set.param_id);
    if (index >= 0) {
        setParamByName(_params[index].name, set.param_value);
        Serial.printf("[PARAMS] %s = %.4f\n", _params[index].name, _params[index].value);
        sendParam(index);
        return;
    }

    Serial.println("[PARAMS] Bilinmeyen parametre!");
}

void ParamManager::_sendPacket(mavlink_message_t& msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    espUart.write(buf, len);
}

#endif  // MAVLINK_PARAMS_ENABLED
