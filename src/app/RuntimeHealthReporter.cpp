#include "RuntimeHealthReporter.h"
#include <cstring>
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
        sendStatusTextThrottled(preflight.firstFailureReason);
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
        sendStatusTextThrottled(_context.sensors->getFaultText());
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

void RuntimeHealthReporter::sendStatusTextThrottled(const char* text) {
    constexpr uint32_t REPEAT_MS = 5000;
    if (!text || text[0] == '\0') {
        return;
    }

    const uint32_t nowMs = millis();
    StatusThrottleSlot* freeSlot = nullptr;
    StatusThrottleSlot* oldestSlot = &_statusThrottleSlots[0];

    for (StatusThrottleSlot& slot : _statusThrottleSlots) {
        if (slot.text[0] == '\0') {
            if (!freeSlot) {
                freeSlot = &slot;
            }
            continue;
        }

        if (strncmp(text, slot.text, sizeof(slot.text)) == 0) {
            if ((uint32_t)(nowMs - slot.lastSentMs) < REPEAT_MS) {
                return;
            }
            slot.lastSentMs = nowMs;
            _context.mavlink->sendStatusText(text);
            return;
        }

        if ((uint32_t)(slot.lastSentMs - oldestSlot->lastSentMs) > 0) {
            continue;
        }
        oldestSlot = &slot;
    }

    StatusThrottleSlot* slot = freeSlot ? freeSlot : oldestSlot;
    strncpy(slot->text, text, sizeof(slot->text) - 1);
    slot->text[sizeof(slot->text) - 1] = '\0';
    slot->lastSentMs = nowMs;
    if (_context.mavlink) {
        _context.mavlink->sendStatusText(text);
    }
}
