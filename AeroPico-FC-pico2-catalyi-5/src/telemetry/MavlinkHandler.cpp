#include "MavlinkHandler.h"
#ifdef MAVLINK_PARAMS_ENABLED
#include "ParamManager.h"
#endif

MavlinkHandler mavlink;

void MavlinkHandler::init(uint32_t baud) {
    espUart.init(baud);
    Serial.println("[MAVLINK] Baslatildi.");
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
        if (flightManager.peekLatest(d)) {
            sendAttitude(d.roll, d.pitch, d.yaw, d.gyroX, d.gyroY, d.gyroZ);
        }
    }

    // RC Channels — 5 Hz
    if (now - _lastRCSent >= STREAM_RC_MS) {
        _lastRCSent = now;
        FlightData d;
        if (flightManager.peekLatest(d)) {
            sendRCChannels(d.aileron, d.elevator, d.throttle, d.rudder);
        }
    }

    // SYS_STATUS — 2 Hz
    if (now - _lastSysStatusSent >= STREAM_SYS_STATUS_MS) {
        _lastSysStatusSent = now;
        sendSysStatus(flightManager.isArmed(), false);
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

            const uint16_t ignore = UINT16_MAX;
            if (rc.chan1_raw == ignore && rc.chan2_raw == ignore &&
                rc.chan3_raw == ignore && rc.chan4_raw == ignore) {
                flightManager.clearRCOverride();
                break;
            }

            uint16_t ch1 = (rc.chan1_raw == ignore) ? flightManager.getAileron()  : rc.chan1_raw;
            uint16_t ch2 = (rc.chan2_raw == ignore) ? flightManager.getElevator() : rc.chan2_raw;
            uint16_t ch3 = (rc.chan3_raw == ignore) ? flightManager.getThrottle() : rc.chan3_raw;
            uint16_t ch4 = (rc.chan4_raw == ignore) ? flightManager.getRudder()   : rc.chan4_raw;
            flightManager.setRCOverride(ch1, ch2, ch3, ch4);
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

void MavlinkHandler::sendSysStatus(bool armed, bool failsafe) {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    (void)armed;
    (void)failsafe;

    // Batarya izleme yok — MAVLink sentinel: bilinmiyor (-1)
    const int8_t battery_remaining = -1;
    const int16_t battery_current  = -1;

    mavlink_msg_sys_status_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        0, 0, 0,
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


bool MavlinkHandler::isESP32Alive() const {
    return _esp32Alive;
}