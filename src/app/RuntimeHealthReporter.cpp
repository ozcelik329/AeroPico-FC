#include "RuntimeHealthReporter.h"
#include "pico/time.h"
#include "../core/events/SystemEventBus.h"
#include "../core/safety/BatteryMonitor.h"
#include "../core/scheduling/SystemTimer.h"
#include "../drivers/Sensors.h"
#include "../telemetry/Blackbox.h"
#include "../telemetry/MavlinkHandler.h"

void RuntimeHealthReporter::init(const RuntimeHealthReporterContext& context) {
    _context = context;
}

bool RuntimeHealthReporter::run(const PreflightResult& preflight,
                                TaskHandle_t sensorTask,
                                TaskHandle_t flightTask,
                                TaskHandle_t telemetryTask) {
    if (!_context.battery || !_context.blackbox || !_context.mavlink ||
        !_context.sensors || !_context.events) {
        return false;
    }

    BatteryStatus battery = _context.battery->evaluate();
    const bool latestBatteryCritical = battery.configured && battery.brownout;

    _runtimeHealth.sensorStackHighWaterWords = sensorTask
        ? clampStackWords(uxTaskGetStackHighWaterMark(sensorTask)) : 0;
    _runtimeHealth.flightStackHighWaterWords = flightTask
        ? clampStackWords(uxTaskGetStackHighWaterMark(flightTask)) : 0;
    _runtimeHealth.telemetryStackHighWaterWords = telemetryTask
        ? clampStackWords(uxTaskGetStackHighWaterMark(telemetryTask)) : 0;
    _runtimeHealth.eventQueueDrops = _context.events->droppedCount() > 0xFFFFu
        ? 0xFFFFu : (uint16_t)_context.events->droppedCount();

    if (!preflight.canArm) {
        _context.mavlink->sendStatusText(preflight.firstFailureReason);
    }

    if (battery.configured && !battery.healthy && !_batteryWarningLatched) {
        _batteryWarningLatched = true;
        _context.events->publish({
            SystemEventType::BatteryWarning,
            micros(),
            battery.brownout ? 2u : 1u
        });
        _context.mavlink->sendStatusText(battery.reason);
    } else if (battery.configured && battery.healthy) {
        _batteryWarningLatched = false;
    }

    if (_context.sensors->getFaultCode() != SensorFaultCode::None) {
        _context.mavlink->sendStatusText(_context.sensors->getFaultText());
    }

    if (!SystemTimer::checkTimingBudgets()) {
        TimingBudgetStatus status = SystemTimer::getTimingBudgetStatus();
        _context.blackbox->logTimingBudget(status);
        _context.mavlink->sendStatusText("Timing budget exceeded");
        _context.events->publish({
            SystemEventType::TimingOverrun,
            micros(),
            ((uint32_t)status.totalDeadlineMisses << 16) | status.totalLoadPermille
        });
    }

    const uint32_t droppedBlackbox = _context.blackbox->droppedRecords();
    _runtimeHealth.blackboxDrops = droppedBlackbox > 0xFFFFu ? 0xFFFFu : (uint16_t)droppedBlackbox;
    _context.blackbox->logRuntimeHealth(_runtimeHealth);
    if (droppedBlackbox != _lastBlackboxDroppedRecords) {
        _lastBlackboxDroppedRecords = droppedBlackbox;
        _context.events->publish({
            SystemEventType::BlackboxDrop,
            micros(),
            droppedBlackbox
        });
        _context.mavlink->sendStatusText("Blackbox records dropped");
    }

    SystemTimer::requestTimingWindowReset();
    return latestBatteryCritical;
}

uint16_t RuntimeHealthReporter::clampStackWords(UBaseType_t value) {
    return value > 0xFFFFu ? 0xFFFFu : (uint16_t)value;
}
