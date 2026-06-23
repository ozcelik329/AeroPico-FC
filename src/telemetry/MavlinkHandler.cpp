#include "MavlinkHandler.h"

MavlinkHandler mavlink;

void MavlinkHandler::init(uint32_t baud) {
    espUart.init(baud);
    Serial.println("[MAVLINK] Baslatildi.");
}

void MavlinkHandler::update() {
    // Gelen byte'ları parse et
    while (MAV_SERIAL.available()) {
        uint8_t byte = MAV_SERIAL.read();
        _parse(byte);
    }

    // ESP32 heartbeat timeout kontrolü — 3 saniye gelmezse ölü say
    if (_esp32Alive && (millis() - _lastESP32Heartbeat > 3000)) {
        _esp32Alive = false;
        Serial.println("[MAVLINK] ESP32 baglantisi kesildi!");
    }

    // Heartbeat gönder — 1 Hz
    if (millis() - _lastHeartbeatSent >= 1000) {
        _lastHeartbeatSent = millis();
        sendHeartbeat();
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
            // İleride: GCS'ten gelen RC override komutları
            mavlink_rc_channels_override_t rc;
            mavlink_msg_rc_channels_override_decode(&msg, &rc);
            // TODO: FlightManager'a ilet
            break;
        }
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

    mavlink_msg_sys_status_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        0, 0, 0,
        0,       // CPU yükü
        0,       // batarya
        -1, -1,  // akım, kalan
        0, 0, 0, 0, 0, 0,
        0, 0, 0  // MAVLink v2 ek parametreler
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAV_SERIAL.write(buf, len);
}


bool MavlinkHandler::isESP32Alive() const {
    return _esp32Alive;
}