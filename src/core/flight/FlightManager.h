#ifndef FLIGHT_MANAGER_H
#define FLIGHT_MANAGER_H

#include <Arduino.h>
#include "../../drivers/IDrivers.h"
#include "../data/SystemBlackboard.h"
#include "../events/SystemEventBus.h"
#include "../rc/RCPipeline.h"
#include "../sensors/SensorPipeline.h"
#include "../safety/FailsafeManager.h"
#include "StatePublisher.h"
#include "../control/ControlPipeline.h"

#include "../safety/ArmDefs.h"

class FlightManager {
  public:
    void init();
    // Initialize FlightManager with concrete drivers (dependency injection)
    void init(IImuDriver* imuDrv, IRxDriver* rxDrv);
    void update();
    void updateSensors();
    void updateRc();
    void publishState();

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
    bool isArmed() const { return __atomic_load_n(&_armedShared, __ATOMIC_ACQUIRE) != 0; }
    bool requestArmFromMavlink(bool arm, bool force, char* reason, size_t reasonLen);
    void setPreflightArmAllowed(bool allowed);
    void setSystemFaults(bool timingExceeded, bool batteryCritical, bool actuatorFault);

    void setRCOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder);
    void clearRCOverride();
    void applyRcMapping(const RcMapping& mapping);

    // Consumer: called by the single consumer (Core 1) to consume pending samples
    bool consumeLatest();
    bool consumeLatest(FlightData& out);

    // Peek latest without consuming — safe for telemetry (secondary reader)
    bool peekLatest(FlightData& out) const;

  private:
    RCPipeline _rcPipeline;
    SensorPipeline _sensorPipeline;
    VehicleState _vehicleState = {};
    RcInputState _rcState = {};

    // Core 1 cache; source is the typed lock-free blackboard topic.
    FlightData _latest = {};

    StatePublisher _statePublisher;
    ControlPipeline _controlPipeline;
    FailsafeManager _failsafeManager;
    bool _preflightArmAllowed = false;
    bool _timingExceeded = false;
    bool _batteryCritical = false;
    bool _actuatorFault = false;
    mutable uint8_t _armedShared = 0;
    SensorHealth _lastPublishedSensorHealth = SensorHealth::Invalid;

    FlightData readLatestSnapshot() const { return _latest; }
    
};

extern FlightManager flightManager;

#endif
