#ifndef MAVLINK_SERVICE_COMMANDS_H
#define MAVLINK_SERVICE_COMMANDS_H

#include <Arduino.h>
#include "../core/safety/PreflightHealth.h"
#include "../drivers/RX.h"
#include "../drivers/Sensors.h"
#include "../storage/CalibrationStorage.h"

struct MavlinkServiceContext {
    SensorManager* sensors = nullptr;
    RXManager* receiver = nullptr;
    ICalibrationStorage* calibrationStorage = nullptr;
    bool* magCalibrationActive = nullptr;
    bool (*isArmed)() = nullptr;
    PreflightResult (*evaluatePreflight)() = nullptr;
    bool (*requestServoTest)(uint8_t surface, uint16_t pulseUs, uint16_t durationMs) = nullptr;
    PreflightResult* lastPreflightResult = nullptr;
};

class MavlinkServiceCommands {
  public:
    void init(const MavlinkServiceContext& context);
    uint8_t handle(uint16_t action, float p2, float p3, float p4, char* reason, size_t reasonLen);

  private:
    MavlinkServiceContext _context = {};

    bool safeForService() const;
    void copyReason(char* reason, size_t reasonLen, const char* text) const;
};

#endif
