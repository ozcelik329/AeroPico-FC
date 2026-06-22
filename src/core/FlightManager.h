#ifndef FLIGHT_MANAGER_H
#define FLIGHT_MANAGER_H

#include <Arduino.h>
#include "pico/mutex.h"
#include "../drivers/Sensors.h"
#include "../drivers/RX.h"
#include "SensorFusion.h"

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

  private:
    SensorManager sensors;
    RXManager rx;
    SensorFusion fusion;

    // Paylaşılan veriler için mutex
    mutex_t _mutex;

    // Korunan veriler (mutex altında okunup yazılır)
    float _roll   = 0.0f;
    float _pitch  = 0.0f;
    float _yaw    = 0.0f;
    float _gyroX  = 0.0f;
    float _gyroY  = 0.0f;
    float _gyroZ  = 0.0f;
    uint16_t _aileron  = 1500;
    uint16_t _elevator = 1500;
    uint16_t _throttle = 1000;
    uint16_t _rudder   = 1500;

    void performSensorFusion();
};

extern FlightManager flightManager;

#endif
