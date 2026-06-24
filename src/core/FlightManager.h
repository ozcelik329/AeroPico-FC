#ifndef FLIGHT_MANAGER_H
#define FLIGHT_MANAGER_H

#include <Arduino.h>
#include "../drivers/Sensors.h"
#include "../drivers/RX.h"
#include "SensorFusion.h"
#include "RingBuffer.h"

#define ARM_THROTTLE_MAX    1050
#define ARM_RUDDER_MIN      1900
#define DISARM_RUDDER_MAX   1100
#define ARM_HOLD_MS         1000

// Core 0 → Core 1 arası paylaşılan veri paketi
struct FlightData {
    float roll, pitch, yaw;
    float gyroX, gyroY, gyroZ;
    uint16_t aileron, elevator, throttle, rudder;
    bool failsafe;
    uint32_t timestamp;
};

class FlightManager {
  public:
    void init();
    void update();

    float getRoll();
    float getPitch();
    float getYaw();
    float getGyroX();
    float getGyroY();
    float getGyroZ();
    uint16_t getAileron();
    uint16_t getElevator();
    uint16_t getThrottle();
    uint16_t getRudder();
    bool isArmed() const { return _armed; }

  private:
    SensorManager sensors;
    RXManager rx;
    SensorFusion fusion;

    // Lock-free ring buffer — 4 slot yeterli (500Hz üretim, 500Hz tüketim)
    RingBuffer<FlightData, 4> _ringBuf;

    // Son okunan veri (Core 1 tarafında cache)
    FlightData _latest = {};

    bool     _armed           = false;
    uint32_t _armHoldStart    = 0;
    uint32_t _disarmHoldStart = 0;

    void performSensorFusion();
    void updateArmDisarm(uint16_t throttle, uint16_t rudder);
};

extern FlightManager flightManager;

#endif