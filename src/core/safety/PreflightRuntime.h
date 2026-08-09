#ifndef PREFLIGHT_RUNTIME_H
#define PREFLIGHT_RUNTIME_H

#include <Arduino.h>
#include "../../types.h"
#include "BatteryMonitor.h"
#include "ModuleSetupRuntime.h"
#include "PreflightHealth.h"

class GpsManager;
class RXManager;
class SensorManager;

struct PreflightRuntimeContext {
    SensorManager* sensors;
    GpsManager* gps;
    RXManager* receiver;
    BatteryMonitor* battery;
    ModuleSetupRuntime* moduleSetup;
    PreflightHealth* health;
};

class PreflightRuntime {
  public:
    void init(const PreflightRuntimeContext& context);
    void setMinSensorQuality(uint8_t minQuality);
    PreflightResult evaluate(uint32_t freeHeapBytes, bool outputsReady, bool timingHealthy);
    bool latestBatteryCritical() const { return _latestBatteryCritical; }

  private:
    bool evaluateSensors();
    bool evaluateModuleSetup();

    PreflightRuntimeContext _context = {};
    uint8_t _minSensorQuality = 60;
    bool _latestBatteryCritical = false;
    char _sensorReason[72] = "Sensor not evaluated";
    char _moduleSetupReason[96] = "Module setup not evaluated";
};

#endif
