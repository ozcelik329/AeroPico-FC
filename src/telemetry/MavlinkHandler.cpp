#include "MavlinkHandler.h"
#ifdef MAVLINK_PARAMS_ENABLED
#include "ParamManager.h"
#endif

MavlinkHandler mavlink;

void MavlinkHandler::init(uint32_t baud) {
    mavlinkTransport.init(baud);
    Serial.println("[MAVLINK] Baslatildi.");
}

void MavlinkHandler::setFlightDataProvider(FlightDataProvider provider) {
    _flightDataProvider = provider;
}

void MavlinkHandler::setArmStateProvider(ArmStateProvider provider) {
    _armStateProvider = provider;
}

void MavlinkHandler::setArmCommandHandler(ArmCommandHandler handler) {
    _armCommandHandler = handler;
}

void MavlinkHandler::setRCOverrideHandler(RCOverrideHandler handler) {
    _rcOverrideHandler = handler;
}

void MavlinkHandler::setClearRCOverrideHandler(ClearRCOverrideHandler handler) {
    _clearRCOverrideHandler = handler;
}

void MavlinkHandler::setRCOverrideEnabled(bool enabled) {
    _rcOverrideEnabled = enabled;
}

void MavlinkHandler::setRCOverrideAllowedWhileArmed(bool allowed) {
    _rcOverrideAllowedWhileArmed = allowed;
}

static uint16_t hzToPeriodMs(uint8_t hz, uint8_t minHz, uint8_t maxHz) {
    if (hz < minHz) hz = minHz;
    if (hz > maxHz) hz = maxHz;
    return (uint16_t)(1000u / hz);
}

void MavlinkHandler::setStreamRates(uint8_t attitudeHz, uint8_t rcHz, uint8_t sysStatusHz) {
    _attitudePeriodMs = hzToPeriodMs(attitudeHz, 1, 50);
    _rcPeriodMs = hzToPeriodMs(rcHz, 1, 25);
    _sysStatusPeriodMs = hzToPeriodMs(sysStatusHz, 1, 10);
}

void MavlinkHandler::update() {
    while (mavlinkTransport.available()) {
        int value = mavlinkTransport.read();
        if (value < 0) {
            break;
        }
        uint8_t byte = (uint8_t)value;
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
    if (now - _lastAttitudeSent >= _attitudePeriodMs) {
        _lastAttitudeSent = now;
        FlightData d;
        if (_flightDataProvider && _flightDataProvider(d)) {
            sendAttitude(d.roll, d.pitch, d.yaw, d.gyroX, d.gyroY, d.gyroZ);
            sendVfrHud(d);
        }
    }

    // RC Channels — 5 Hz
    if (now - _lastRCSent >= _rcPeriodMs) {
        _lastRCSent = now;
        FlightData d;
        if (_flightDataProvider && _flightDataProvider(d)) {
            sendRCChannels(d.aileron, d.elevator, d.throttle, d.rudder);
        }
    }

    // SYS_STATUS — 2 Hz
    if (now - _lastSysStatusSent >= _sysStatusPeriodMs) {
        _lastSysStatusSent = now;
        FlightData d = {};
        bool hasData = _flightDataProvider && _flightDataProvider(d);
        bool armed = _armStateProvider ? _armStateProvider() : false;
        sendSysStatus(
            armed,
            hasData ? d.failsafe : true,
            hasData ? d.sensorHealth : SensorHealth::Invalid
        );
        sendGpsRawIntNoGps();
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
            if (!_targetsThisVehicle(rc.target_system, rc.target_component)) {
                break;
            }
            if (!_rcOverrideEnabled) {
                sendStatusText("RC override rejected", MAV_SEVERITY_WARNING);
                break;
            }
            if (!_rcOverrideAllowedWhileArmed && _armStateProvider && _armStateProvider()) {
                sendStatusText("RC override rejected while armed", MAV_SEVERITY_WARNING);
                break;
            }
            _applyRCOverrideRaw(rc.chan1_raw, rc.chan2_raw, rc.chan3_raw, rc.chan4_raw);
            break;
        }
        case MAVLINK_MSG_ID_COMMAND_LONG:
            _handleCommandLong(msg);
            break;
        case MAVLINK_MSG_ID_MISSION_REQUEST_LIST: {
            mavlink_mission_request_list_t request;
            mavlink_msg_mission_request_list_decode(&msg, &request);
            if (_targetsThisVehicle(request.target_system, request.target_component)) {
                sendMissionCountZero(msg.sysid, msg.compid);
            }
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

bool MavlinkHandler::_targetsThisVehicle(uint8_t targetSystem, uint8_t targetComponent) const {
    if (targetSystem != 0 && targetSystem != MAV_SYSTEM_ID) {
        return false;
    }
    if (targetComponent != 0 && targetComponent != MAV_COMPONENT_ID) {
        return false;
    }
    return true;
}

void MavlinkHandler::_handleCommandLong(const mavlink_message_t& msg) {
    mavlink_command_long_t command;
    mavlink_msg_command_long_decode(&msg, &command);
    if (!_targetsThisVehicle(command.target_system, command.target_component)) {
        return;
    }

    if (command.command != MAV_CMD_COMPONENT_ARM_DISARM) {
        sendCommandAck(command.command, MAV_RESULT_UNSUPPORTED);
        return;
    }

    const bool arm = command.param1 >= 0.5f;
    const bool force = command.param2 == 21196.0f;
    char reason[50] = {};
    bool accepted = false;
    if (_armCommandHandler) {
        accepted = _armCommandHandler(arm, force, reason, sizeof(reason));
    } else {
        strncpy(reason, "arm command unavailable", sizeof(reason) - 1);
    }

    sendCommandAck(command.command, accepted ? MAV_RESULT_ACCEPTED : MAV_RESULT_DENIED);
    if (!accepted && reason[0] != '\0') {
        sendStatusText(reason, MAV_SEVERITY_WARNING);
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

void MavlinkHandler::handleRCOverrideMessageForTest(uint8_t targetSystem, uint8_t targetComponent,
                                                    uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4) {
    mavlink_message_t msg;
    mavlink_msg_rc_channels_override_pack(
        MAV_SYSTEM_ID,
        MAV_COMPONENT_ID,
        &msg,
        targetSystem,
        targetComponent,
        ch1,
        ch2,
        ch3,
        ch4,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX
    );
    _handleMessage(msg);
}
#endif

void MavlinkHandler::sendHeartbeat() {
    mavlink_message_t msg;
    const bool armed = _armStateProvider ? _armStateProvider() : false;
    uint8_t baseMode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    if (armed) {
        baseMode |= MAV_MODE_FLAG_SAFETY_ARMED;
    }

    mavlink_msg_heartbeat_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        MAV_TYPE_FIXED_WING,
        MAV_AUTOPILOT_GENERIC,
        baseMode,
        0,
        MAV_STATE_ACTIVE
    );

    _sendMessage(msg);
}

void MavlinkHandler::sendAttitude(float roll, float pitch, float yaw,
                                   float rollRate, float pitchRate, float yawRate) {
    mavlink_message_t msg;

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

    _sendMessage(msg);
}

void MavlinkHandler::sendRCChannels(uint16_t ch1, uint16_t ch2,
                                     uint16_t ch3, uint16_t ch4) {
    mavlink_message_t msg;

    mavlink_msg_rc_channels_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        millis(), 4,
        ch1, ch2, ch3, ch4,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0
    );

    _sendMessage(msg);
}

void MavlinkHandler::sendSysStatus(bool armed, bool failsafe, SensorHealth sensorHealth) {
    mavlink_message_t msg;

    // Batarya izleme yok — MAVLink sentinel: bilinmiyor (-1)
    const int8_t battery_remaining = -1;
    const int16_t battery_current  = -1;
    const uint32_t sensorBits =
        MAV_SYS_STATUS_SENSOR_3D_GYRO |
        MAV_SYS_STATUS_SENSOR_3D_ACCEL |
        MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE |
        MAV_SYS_STATUS_SENSOR_3D_MAG;
    const bool sensorsHealthy = sensorHealth == SensorHealth::Ok;
    const uint32_t healthBits = (sensorsHealthy && !failsafe) ? sensorBits : 0;
    const uint16_t loadPermille = armed ? 500 : 0;
    const uint16_t commDropPermille = failsafe ? 1000 : 0;

    mavlink_msg_sys_status_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        sensorBits,
        sensorBits,
        healthBits,
        loadPermille,
        0,
        battery_current,
        battery_remaining,
        commDropPermille, 0, 0, 0, 0, 0,
        0, 0, 0
    );

    _sendMessage(msg);
}

void MavlinkHandler::sendVfrHud(const FlightData& data) {
    mavlink_message_t msg;
    int16_t heading = (int16_t)data.yaw;
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    const uint16_t throttleUs = constrain(data.throttle, (uint16_t)PWM_MIN, (uint16_t)PWM_MAX);
    const int16_t throttlePercent = (int16_t)(((uint32_t)(throttleUs - PWM_MIN) * 100u) /
                                              (PWM_MAX - PWM_MIN));

    mavlink_msg_vfr_hud_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        0.0f,
        0.0f,
        heading,
        throttlePercent,
        data.altitudeM,
        data.verticalSpeedMps
    );
    _sendMessage(msg);
}

void MavlinkHandler::sendGpsRawIntNoGps() {
    mavlink_message_t msg;
    mavlink_msg_gps_raw_int_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        (uint64_t)micros(),
        GPS_FIX_TYPE_NO_GPS,
        0,
        0,
        0,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        UINT16_MAX,
        0,
        0,
        UINT32_MAX,
        UINT32_MAX,
        UINT32_MAX,
        UINT32_MAX,
        0
    );
    _sendMessage(msg);
}

void MavlinkHandler::sendMissionCountZero(uint8_t targetSystem, uint8_t targetComponent) {
    mavlink_message_t msg;
    mavlink_msg_mission_count_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        targetSystem,
        targetComponent,
        0,
        MAV_MISSION_TYPE_MISSION,
        0
    );
    _sendMessage(msg);
}

void MavlinkHandler::sendCommandAck(uint16_t command, uint8_t result) {
    mavlink_message_t msg;
    mavlink_msg_command_ack_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        command,
        result,
        0,
        0,
        0,
        0
    );
    _sendMessage(msg);
}

void MavlinkHandler::sendStatusText(const char* text, uint8_t severity) {
    mavlink_message_t msg;

    char statustext[50] = {};
    strncpy(statustext, text, sizeof(statustext) - 1);

    mavlink_msg_statustext_pack(
        MAV_SYSTEM_ID, MAV_COMPONENT_ID, &msg,
        severity,
        statustext,
        0,
        0
    );

    _sendMessage(msg);
}

bool MavlinkHandler::isESP32Alive() const {
    return _esp32Alive;
}

void MavlinkHandler::_sendMessage(mavlink_message_t& msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    mavlinkTransport.writePacket(buf, len);
}
