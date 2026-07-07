#include "MavlinkHandler.h"
#ifdef MAVLINK_PARAMS_ENABLED
#include "ParamManager.h"
#endif

MavlinkHandler mavlink;

void MavlinkHandler::init(uint32_t baud) {
    espUart.init(baud);
    Serial.println("[MAVLINK] Baslatildi.");
}

void MavlinkHandler::setFlightDataProvider(FlightDataProvider provider) {
    _flightDataProvider = provider;
}

void MavlinkHandler::setArmStateProvider(ArmStateProvider provider) {
    _armStateProvider = provider;
}

void MavlinkHandler::setRCOverrideHandler(RCOverrideHandler handler) {
    _rcOverrideHandler = handler;
}

void MavlinkHandler::setClearRCOverrideHandler(ClearRCOverrideHandler handler) {
    _clearRCOverrideHandler = handler;
}

void MavlinkHandler::update() {
    while (MAV_SERIAL.available()) {
        uint8_t byte = MAV_SERIAL.read();
        _parse(byte);
    }

    uint32_t now = millis();

    // ESP32 heartbeat timeout — 3 saniye
    if (_esp32Alive && (now - _lastESP32Heartbeat > 3000)) {
        _esp32Alive = false;
        Serial.println("[MAVLINK] ESP32 baglantisi kesildi!");
    }

    // Heartbeat — 1 Hz
    if (now - _lastHeartbeatSent >= STREAM_HEARTBEAT_MS) {
        _lastHeartbeatSent = now;
        sendHeartbeat();
    }

    // Attitude — 10 Hz
    if (now - _lastAttitudeSent >= STREAM_ATTITUDE_MS) {
        _lastAttitudeSent = now;
        FlightData d;
        if (_flightDataProvider && _flightDataProvider(d)) {
            sendAttitude(d.roll, d.pitch, d.yaw, d.gyroX, d.gyroY, d.gyroZ);
        }
    }

    // RC Channels — 5 Hz
    if (now - _lastRCSent >= STREAM_RC_MS) {
        _lastRCSent = now;
        FlightData d;
        if (_flightDataProvider && _flightDataProvider(d)) {
            sendRCChannels(d.aileron, d.elevator, d.throttle, d.rudder);
        }
    }

    // SYS_STATUS — 2 Hz
    if (now - _lastSysStatusSent >= STREAM_SYS_STATUS_MS) {
        _lastSysStatusSent = now;
        FlightData d = {};
        bool hasData = _flightDataProvider && _flightDataProvider(d);
        bool armed = _armStateProvider ? _armStateProvider() : false;
        sendSysStatus(
            armed,
            hasData ? d.failsafe : true,
            hasData ? d.sensorHealth : SensorHealth::Invalid
        );
    }
}

void MavlinkHandler::_parse(uint8_t byte) {
    if (mavlink_parse_char(MAVLINK_COMM_0, byte, &_msg, &_status)) {
        _handleMessage(_msg);
    }
}

void MavlinkHandler::_handleMessage(mavlink_message_t& msg) {
    switch (msg.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            _lastESP32Heartbeat = millis();
            if (!_esp32Alive) {
                _esp32Alive = true;
                Serial.println("[MAVLINK] ESP32 baglandi.");
            }
            break;
        }
        case MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE: {
            mavlink_rc_channels_override_t rc;
            mavlink_msg_rc_channels_override_decode(&msg, &rc);
            _applyRCOverrideRaw(rc.chan1_raw, rc.chan2_raw, rc.chan3_raw, rc.chan4_raw);
            break;
        }
#ifdef MAVLINK_PARAMS_ENABLED
        case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
        case MAVLINK_MSG_ID_PARAM_SET:
            paramManager.handleMessage(msg);
            break;
#endif
        default:
            break;
    }
}

void MavlinkHandler::_applyRCOverrideRaw(uint16_t rawCh1, uint16_t rawCh2, uint16_t rawCh3, uint16_t rawCh4) {
    const uint16_t ignore = UINT16_MAX;
    if (rawCh1 == ignore && rawCh2 == ignore && rawCh3 == ignore && rawCh4 == ignore) {
        if (_clearRCOverrideHandler) {
            _clearRCOverrideHandler();
        }
        return;
    }

    FlightData d = {};
    bool hasData = _flightDataProvider && _flightDataProvider(d);
    uint16_t ch1 = (rawCh1 == ignore && hasData) ? d.aileron  : rawCh1;
    uint16_t ch2 = (rawCh2 == ignore && hasData) ? d.elevator : rawCh2;
    uint16_t ch3 = (rawCh3 == ignore && hasData) ? d.throttle : rawCh3;
    uint16_t ch4 = (rawCh4 == ignore && hasData) ? d.rudder   : rawCh4;

    if (rawCh1 == ignore && !hasData) ch1 = PWM_NEUTRAL;
    if (rawCh2 == ignore && !hasData) ch2 = PWM_NEUTRAL;
    if (rawCh3 == ignore && !hasData) ch3 = PWM_MIN;
    if (rawCh4 == ignore && !hasData) ch4 = PWM_NEUTRAL;

    if (_rcOverrideHandler) {
        _rcOverrideHandler(ch1, ch2, ch3, ch4);
    }
}

#ifdef UNIT_TEST
void MavlinkHandler::handleRCOverrideForTest(uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4) {
    _applyRCOverrideRaw(ch1, ch2, ch3, ch4);
}
#endif

void MavlinkHandler::sendHeartbeat() {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    mavlink_msg_heartbeat_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        MAV_TYPE_FIXED_WING,
        MAV_AUTOPILOT_GENERIC,
        MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
        0,
        MAV_STATE_ACTIVE
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAV_SERIAL.write(buf, len);
}

void MavlinkHandler::sendAttitude(float roll, float pitch, float yaw,
                                   float rollRate, float pitchRate, float yawRate) {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    mavlink_msg_attitude_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        millis(),
        roll  * DEG_TO_RAD,
        pitch * DEG_TO_RAD,
        yaw   * DEG_TO_RAD,
        rollRate  * DEG_TO_RAD,
        pitchRate * DEG_TO_RAD,
        yawRate   * DEG_TO_RAD
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAV_SERIAL.write(buf, len);
}

void MavlinkHandler::sendRCChannels(uint16_t ch1, uint16_t ch2,
                                     uint16_t ch3, uint16_t ch4) {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    mavlink_msg_rc_channels_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        millis(), 4,
        ch1, ch2, ch3, ch4,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAV_SERIAL.write(buf, len);
}

void MavlinkHandler::sendSysStatus(bool armed, bool failsafe, SensorHealth sensorHealth) {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    (void)armed;
    (void)failsafe;

    // Batarya izleme yok — MAVLink sentinel: bilinmiyor (-1)
    const int8_t battery_remaining = -1;
    const int16_t battery_current  = -1;
    const uint32_t sensorBits =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |
        MAV_SYS_STATUS_SENSOR_3D_ACCEL |
        MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE |
        MAV_SYS_STATUS_SENSOR_3D_MAG;
    const bool sensorsHealthy = sensorHealth == SensorHealth::Ok;
    const uint32_t healthBits = sensorsHealthy ? sensorBits : 0;

    mavlink_msg_sys_status_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        sensorBits,
        sensorBits,
        healthBits,
        0,
        0,
        battery_current,
        battery_remaining,
        0, 0, 0, 0, 0, 0,
        0, 0, 0
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAV_SERIAL.write(buf, len);
}

void MavlinkHandler::sendStatusText(const char* text, uint8_t severity) {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    char statustext[50] = {};
    strncpy(statustext, text, sizeof(statustext) - 1);

    mavlink_msg_statustext_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        severity,
        statustext,
        0,
        0
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAV_SERIAL.write(buf, len);
}

bool MavlinkHandler::isESP32Alive() const {
    return _esp32Alive;
}
