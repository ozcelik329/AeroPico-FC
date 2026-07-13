#include "ParamManager.h"
#include "MavlinkHandler.h"
#include "MavlinkTransport.h"

#ifdef MAVLINK_PARAMS_ENABLED

ParamManager paramManager;

void ParamManager::init() {
    loadPersistent();
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

void ParamManager::setRcMappingApplyHandler(RcMappingApplyHandler handler) {
    _rcMappingApplyHandler = handler;
}

void ParamManager::setMavlinkRatesApplyHandler(MavlinkRatesApplyHandler handler) {
    _mavlinkRatesApplyHandler = handler;
}

void ParamManager::setBlackboxRateApplyHandler(BlackboxRateApplyHandler handler) {
    _blackboxRateApplyHandler = handler;
}

void ParamManager::setPreflightQualityApplyHandler(PreflightQualityApplyHandler handler) {
    _preflightQualityApplyHandler = handler;
}

void ParamManager::setArmStateProvider(ArmStateProvider provider) {
    _armStateProvider = provider;
}

void ParamManager::setStorage(IParamStorage* storage) {
    _storage = storage;
}

bool ParamManager::_isPidParam(uint8_t index) const {
    return index <= PARAM_IDX_RATE_D;
}

bool ParamManager::_isMixerParam(uint8_t index) const {
    return index >= PARAM_IDX_MIX_ROLL && index <= PARAM_IDX_SERVO_MAX;
}

bool ParamManager::_isRcMappingParam(uint8_t index) const {
    return (index >= PARAM_IDX_RC_ROLL_CH && index <= PARAM_IDX_RC_YAW_CH) ||
           index == PARAM_IDX_RC_MODE_CH;
}

bool ParamManager::_isMavlinkRateParam(uint8_t index) const {
    return index >= PARAM_IDX_MAV_ATT_HZ && index <= PARAM_IDX_MAV_SYS_HZ;
}

int ParamManager::_findParamIndex(const char* name) const {
    for (uint8_t i = 0; i < PARAM_COUNT; i++) {
        if (strncmp(_params[i].name, name, 16) == 0) {
            return i;
        }
    }
    return -1;
}

void ParamManager::_applyParam(uint8_t index) {
    if (_isPidParam(index) && _pidGainsApplyHandler) {
        _pidGainsApplyHandler(
            getAngleP(), getAngleI(), getAngleD(),
            getRateP(), getRateI(), getRateD()
        );
    }
    if (_isMixerParam(index) && _mixerSettingsApplyHandler) {
        _mixerSettingsApplyHandler(getMixerSettings());
    }
    if (index == PARAM_IDX_FS_TIMEOUT && _failsafeTimeoutApplyHandler) {
        _failsafeTimeoutApplyHandler(getFailsafeTimeoutMs());
    }
    if (_isRcMappingParam(index) && _rcMappingApplyHandler) {
        _rcMappingApplyHandler(
            getRcRollChannel(),
            getRcPitchChannel(),
            getRcThrottleChannel(),
            getRcYawChannel(),
            getRcModeChannel()
        );
    }
    if (_isMavlinkRateParam(index) && _mavlinkRatesApplyHandler) {
        _mavlinkRatesApplyHandler(
            getMavlinkAttitudeHz(),
            getMavlinkRcHz(),
            getMavlinkSysStatusHz()
        );
    }
    if (index == PARAM_IDX_BB_LOG_HZ && _blackboxRateApplyHandler) {
        _blackboxRateApplyHandler(getBlackboxLogHz());
    }
    if (index == PARAM_IDX_PREF_Q_MIN && _preflightQualityApplyHandler) {
        _preflightQualityApplyHandler(getPreflightMinQuality());
    }
}

bool ParamManager::loadPersistent() {
    if (!_storage) return false;

    ParamStorageBlob blob = {};
    if (!_storage->load(blob) || !ParamStorage::hasValidEnvelope(blob)) {
        return false;
    }
    if (blob.count > PARAM_PERSISTED_COUNT) {
        return false;
    }

    for (uint8_t i = 0; i < blob.count; i++) {
        _params[i].value = constrain(blob.values[i], _params[i].minVal, _params[i].maxVal);
        _applyParam(i);
    }
    _dirty = blob.count != PARAM_PERSISTED_COUNT;
    _params[PARAM_IDX_SAVE].value = 0.0f;
    Serial.println("[PARAMS] Kalici parametreler yuklendi.");
    return true;
}

bool ParamManager::savePersistent() {
    _lastError = "";
    if (!_storage) {
        _lastError = "Parameter storage unavailable";
        return false;
    }
    if (_armStateProvider && _armStateProvider()) {
        _lastError = "Parameter save rejected while armed";
        return false;
    }

    float values[PARAM_PERSISTED_COUNT];
    for (uint8_t i = 0; i < PARAM_PERSISTED_COUNT; i++) {
        values[i] = _params[i].value;
    }

    ParamStorageBlob blob = ParamStorage::makeBlob(values, PARAM_PERSISTED_COUNT);
    if (!_storage->save(blob)) {
        _lastError = "Parameter flash write failed";
        return false;
    }

    _dirty = false;
    _params[PARAM_IDX_SAVE].value = 0.0f;
    Serial.println("[PARAMS] Parametreler flash'a kaydedildi.");
    return true;
}

bool ParamManager::setParamByName(const char* name, float value) {
    int index = _findParamIndex(name);
    if (index < 0) return false;

    if (index == PARAM_IDX_SAVE) {
        _params[PARAM_IDX_SAVE].value = 0.0f;
        return value >= 0.5f ? savePersistent() : true;
    }

    if (_armStateProvider && _armStateProvider()) {
        _lastError = "Parameter change rejected while armed";
        return false;
    }

    _lastError = "";
    _params[index].value = constrain(value, _params[index].minVal, _params[index].maxVal);
    _dirty = index < PARAM_PERSISTED_COUNT;
    _applyParam((uint8_t)index);
    return true;
}

MixerSettings ParamManager::getMixerSettings() const {
    MixerSettings settings;
    settings.rollGain = _params[PARAM_IDX_MIX_ROLL].value;
    settings.pitchGain = _params[PARAM_IDX_MIX_PITCH].value;
    settings.yawGain = _params[PARAM_IDX_MIX_YAW].value;
    settings.aileronTrim = (int)_params[PARAM_IDX_TRIM_AIL].value;
    settings.elevatorTrim = (int)_params[PARAM_IDX_TRIM_ELE].value;
    settings.rudderTrim = (int)_params[PARAM_IDX_TRIM_RUD].value;
    settings.throttleTrim = (int)_params[PARAM_IDX_TRIM_THR].value;
    settings.reverseAileron = _params[PARAM_IDX_REV_AIL].value >= 0.5f;
    settings.reverseElevator = _params[PARAM_IDX_REV_ELE].value >= 0.5f;
    settings.reverseRudder = _params[PARAM_IDX_REV_RUD].value >= 0.5f;
    settings.servoMin = (int)_params[PARAM_IDX_SERVO_MIN].value;
    settings.servoMax = (int)_params[PARAM_IDX_SERVO_MAX].value;
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
    _sendIndex = 0;
    _nextSendMs = 0;
    _sendActive = true;
}

void ParamManager::processSendQueue(uint32_t nowMs) {
    if (!_sendActive || (int32_t)(nowMs - _nextSendMs) < 0) {
        return;
    }

    sendParam(_sendIndex++);
    if (_sendIndex >= PARAM_COUNT) {
        _sendActive = false;
        return;
    }
    constexpr uint32_t PARAM_SEND_INTERVAL_MS = 20;
    _nextSendMs = nowMs + PARAM_SEND_INTERVAL_MS;
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
    mavlinkTransport.writePacket(buf, len);
}

void ParamManager::_handleParamRequestList(const mavlink_message_t& msg) {
    mavlink_param_request_list_t request;
    mavlink_msg_param_request_list_decode(&msg, &request);
    if (request.target_system != 0 && request.target_system != MAV_SYSTEM_ID) return;
    if (request.target_component != 0 && request.target_component != MAV_COMPONENT_ID) return;
    Serial.println("[PARAMS] GCS parametre listesi istedi.");
    sendAll();
}

void ParamManager::_handleParamSet(const mavlink_message_t& msg) {
    mavlink_param_set_t set;
    mavlink_msg_param_set_decode(&msg, &set);
    if (set.target_system != 0 && set.target_system != MAV_SYSTEM_ID) return;
    if (set.target_component != 0 && set.target_component != MAV_COMPONENT_ID) return;

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
    mavlinkTransport.writePacket(buf, len);
}

#endif  // MAVLINK_PARAMS_ENABLED
