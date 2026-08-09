#ifndef RUNTIME_HEALTH_REPORTER_H
#define RUNTIME_HEALTH_REPORTER_H

#include <FreeRTOS.h>
#include <task.h>
#include "../types.h"
#include "../core/safety/PreflightHealth.h"

class BatteryMonitor;
class Blackbox;
class MavlinkHandler;
class SensorManager;
class SystemEventBus;

struct RuntimeHealthReporterContext {
    BatteryMonitor* battery;
    Blackbox* blackbox;
    MavlinkHandler* mavlink;
    SensorManager* sensors;
    SystemEventBus* events;
};

class RuntimeHealthReporter {
  public:
    void init(const RuntimeHealthReporterContext& context);
    bool run(const PreflightResult& preflight,
             TaskHandle_t sensorTask,
             TaskHandle_t flightTask,
             TaskHandle_t telemetryTask);

  private:
    static uint16_t clampStackWords(UBaseType_t value);

    RuntimeHealthReporterContext _context = {};
    RuntimeHealthStatus _runtimeHealth = {};
    uint32_t _lastBlackboxDroppedRecords = 0;
    bool _batteryWarningLatched = false;
};

#endif
