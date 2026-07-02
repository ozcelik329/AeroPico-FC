#ifndef FLIGHT_MANAGER_H
#define FLIGHT_MANAGER_H

#include <Arduino.h>
#include "../drivers/IDrivers.h"
#include "SensorFusion.h"
#include "ThreadSafeRingBuffer.h"
#include "FlightModeController.h"
#include "NavigationController.h"
#include "AltitudeController.h"

#include "ArmDefs.h"

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
    // Initialize FlightManager with concrete drivers (dependency injection)
    void init(IImuDriver* imuDrv, IRxDriver* rxDrv);
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
    bool isArmed() { return _modeController.isArmed(); }

    // Consumer: called by the single consumer (Core 1) to consume pending samples
    void consumeLatest();

    // Peek latest without consuming — safe for telemetry (secondary reader)
    bool peekLatest(FlightData& out) const;

  private:
    // Injected drivers (do not instantiate concrete types here)
    IImuDriver* _imu = nullptr;
    IRxDriver*  _rx  = nullptr;
    SensorFusion fusion;

    // Thread-safe ring buffer for multi-producer / multi-consumer usage
    ThreadSafeRingBuffer<FlightData, 4> _ringBuf;

    // Son okunan veri (Core 1 tarafında cache)
    FlightData _latest = {};

    // Controllers — FlightManager only orchestrates
    FlightModeController _modeController;
    NavigationController _navController;
    AltitudeController  _altController;

    void performSensorFusion();
    void updateControllers(const FlightData& data);

    // Sequence counter to provide atomic-like snapshot semantics for `_latest`
    volatile uint32_t _latest_seq = 0;
    FlightData readLatestSnapshot() const;
    
};

extern FlightManager flightManager;

#endif