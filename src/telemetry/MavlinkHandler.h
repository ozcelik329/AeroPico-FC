#ifndef MAVLINK_HANDLER_H
#define MAVLINK_HANDLER_H

#include <Arduino.h>
#include <common/mavlink.h>
#include "../drivers/PioUart.h"

// Sistem tanımları
#define MAV_SYSTEM_ID    1
#define MAV_COMPONENT_ID MAV_COMP_ID_AUTOPILOT1
#define MAV_SERIAL       espUart   // ESP32-CAM UART bağlantısı

class MavlinkHandler {
  public:
    void init(uint32_t baud = 57600);
    void update();  // Gelen paketleri oku + parse et

    // Gönderme
    void sendHeartbeat();
    void sendAttitude(float roll, float pitch, float yaw,
                      float rollRate, float pitchRate, float yawRate);
    void sendRCChannels(uint16_t ch1, uint16_t ch2,
                        uint16_t ch3, uint16_t ch4);
    void sendSysStatus(bool armed, bool failsafe);

    // Durum
    bool isESP32Alive() const;   // ESP32 heartbeat geliyor mu?

  private:
    void _parse(uint8_t byte);
    void _handleMessage(mavlink_message_t& msg);

    mavlink_message_t _msg;
    mavlink_status_t  _status;

    uint32_t _lastESP32Heartbeat = 0;
    bool     _esp32Alive         = false;

    uint32_t _lastHeartbeatSent  = 0;
};

extern MavlinkHandler mavlink;

#endif