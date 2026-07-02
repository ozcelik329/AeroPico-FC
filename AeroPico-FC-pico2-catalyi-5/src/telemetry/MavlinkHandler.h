#ifndef MAVLINK_HANDLER_H
#define MAVLINK_HANDLER_H

#include <Arduino.h>
#include <common/mavlink.h>
#include "../drivers/PioUart.h"
#include "../core/FlightManager.h"

#define MAV_SYSTEM_ID    1
#define MAV_COMPONENT_ID MAV_COMP_ID_AUTOPILOT1
#define MAV_SERIAL       espUart

// Stream frekansları (ms)
#define STREAM_HEARTBEAT_MS   1000  // 1 Hz
#define STREAM_ATTITUDE_MS     100  // 10 Hz
#define STREAM_RC_MS           200  // 5 Hz
#define STREAM_SYS_STATUS_MS   500  // 2 Hz

class MavlinkHandler {
  public:
    void init(uint32_t baud = 57600);
    void update();

    void sendHeartbeat();
    void sendAttitude(float roll, float pitch, float yaw,
                      float rollRate, float pitchRate, float yawRate);
    void sendRCChannels(uint16_t ch1, uint16_t ch2,
                        uint16_t ch3, uint16_t ch4);
    void sendSysStatus(bool armed, bool failsafe);

    bool isESP32Alive() const;

  private:
    void _parse(uint8_t byte);
    void _handleMessage(mavlink_message_t& msg);

    mavlink_message_t _msg;
    mavlink_status_t  _status;

    uint32_t _lastESP32Heartbeat  = 0;
    bool     _esp32Alive          = false;

    // Stream zamanlayıcıları
    uint32_t _lastHeartbeatSent   = 0;
    uint32_t _lastAttitudeSent    = 0;
    uint32_t _lastRCSent          = 0;
    uint32_t _lastSysStatusSent   = 0;
};

extern MavlinkHandler mavlink;

#endif