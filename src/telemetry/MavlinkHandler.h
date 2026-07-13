#ifndef MAVLINK_HANDLER_H
#define MAVLINK_HANDLER_H

#include <Arduino.h>
#include <common/mavlink.h>
#include "../types.h"
#include "AeroPicoMavlinkCommands.h"
#include "MavlinkTransport.h"

#define MAV_SYSTEM_ID    1
#define MAV_COMPONENT_ID MAV_COMP_ID_AUTOPILOT1

// Stream frekansları (ms)
#define STREAM_HEARTBEAT_MS        1000  // 1 Hz
#define STREAM_ATTITUDE_DEFAULT_MS  100  // 10 Hz
#define STREAM_RC_DEFAULT_MS        200  // 5 Hz
#define STREAM_SYS_DEFAULT_MS       500  // 2 Hz

class MavlinkHandler {
  public:
    using FlightDataProvider = bool (*)(FlightData& out);
    using ArmStateProvider = bool (*)();
    using ArmCommandHandler = bool (*)(bool arm, bool force, char* reason, size_t reasonLen);
    using ServiceCommandHandler = uint8_t (*)(uint16_t action, float p2, float p3, float p4,
                                              char* reason, size_t reasonLen);
    using RCOverrideHandler = void (*)(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder);
    using ClearRCOverrideHandler = void (*)();

    void init(uint32_t baud = 57600);
    void update();
    void setFlightDataProvider(FlightDataProvider provider);
    void setArmStateProvider(ArmStateProvider provider);
    void setArmCommandHandler(ArmCommandHandler handler);
    void setServiceCommandHandler(ServiceCommandHandler handler);
    void setRCOverrideHandler(RCOverrideHandler handler);
    void setClearRCOverrideHandler(ClearRCOverrideHandler handler);
    void setRCOverrideEnabled(bool enabled);
    void setRCOverrideAllowedWhileArmed(bool allowed);
    void setStreamRates(uint8_t attitudeHz, uint8_t rcHz, uint8_t sysStatusHz);

    void sendHeartbeat();
    void sendAttitude(float roll, float pitch, float yaw,
                      float rollRate, float pitchRate, float yawRate);
    void sendRCChannels(uint16_t ch1, uint16_t ch2,
                        uint16_t ch3, uint16_t ch4);
    void sendSysStatus(bool armed, bool failsafe, SensorHealth sensorHealth);
    void sendVfrHud(const FlightData& data);
    void sendGpsRawIntNoGps();
    void sendMissionCountZero(uint8_t targetSystem, uint8_t targetComponent);
    void sendCommandAck(uint16_t command, uint8_t result);
    void sendStatusText(const char* text, uint8_t severity = MAV_SEVERITY_WARNING);

    bool isESP32Alive() const;

#ifdef UNIT_TEST
    void handleRCOverrideForTest(uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4);
    void handleRCOverrideMessageForTest(uint8_t targetSystem, uint8_t targetComponent,
                                        uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4);
    void handleMessageForTest(mavlink_message_t& msg) { _handleMessage(msg); }
#endif

  private:
    void _parse(uint8_t byte);
    void _handleMessage(mavlink_message_t& msg);
    void _applyRCOverrideRaw(uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4);
    void _handleCommandLong(const mavlink_message_t& msg);
    bool _targetsThisVehicle(uint8_t targetSystem, uint8_t targetComponent) const;
    void _sendMessage(mavlink_message_t& msg);

    mavlink_message_t _msg;
    mavlink_status_t  _status;

    uint32_t _lastESP32Heartbeat  = 0;
    bool     _esp32Alive          = false;
    FlightDataProvider _flightDataProvider = nullptr;
    ArmStateProvider _armStateProvider = nullptr;
    ArmCommandHandler _armCommandHandler = nullptr;
    ServiceCommandHandler _serviceCommandHandler = nullptr;
    RCOverrideHandler _rcOverrideHandler = nullptr;
    ClearRCOverrideHandler _clearRCOverrideHandler = nullptr;
    bool _rcOverrideEnabled = false;
    bool _rcOverrideAllowedWhileArmed = false;

    // Stream zamanlayıcıları
    uint32_t _lastHeartbeatSent   = 0;
    uint32_t _lastAttitudeSent    = 0;
    uint32_t _lastRCSent          = 0;
    uint32_t _lastSysStatusSent   = 0;
    uint16_t _attitudePeriodMs    = STREAM_ATTITUDE_DEFAULT_MS;
    uint16_t _rcPeriodMs          = STREAM_RC_DEFAULT_MS;
    uint16_t _sysStatusPeriodMs   = STREAM_SYS_DEFAULT_MS;
};

extern MavlinkHandler mavlink;

#endif
