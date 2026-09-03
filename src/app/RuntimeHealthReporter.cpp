#include "RuntimeHealthReporter.h"
#include <cstring>
#include "pico/time.h"
#include "../core/events/SystemEventBus.h"
#include "../core/safety/BatteryMonitor.h"
#include "../core/safety/FailsafeManager.h"
#include "../core/scheduling/SystemTimer.h"
#include "../drivers/Sensors.h"
#include "../telemetry/Blackbox.h"
#include "../telemetry/MavlinkHandler.h"

static const char* sensorFaultToken(SensorFaultCode fault) {
    switch (fault) {
        case SensorFaultCode::None: return "NONE";
        case SensorFaultCode::I2cWhoamiWriteFailed: return "WHOAMI_WR";
        case SensorFaultCode::I2cWhoamiReadFailed: return "WHOAMI_RD";
        case SensorFaultCode::WhoamiMismatch: return "WHOAMI_BAD";
        case SensorFaultCode::I2cRawWriteFailed: return "RAW_WR";
        case SensorFaultCode::I2cRawReadFailed: return "RAW_RD";
        case SensorFaultCode::DmaChannelClaimFailed: return "DMA_CLAIM";
        case SensorFaultCode::DmaTransferTimeout: return "DMA_TIMEOUT";
        case SensorFaultCode::AuxI2cWriteFailed: return "AUX_WR";
        case SensorFaultCode::AuxDmaTransferTimeout: return "AUX_DMA_TIMEOUT";
        case SensorFaultCode::AuxPollingFallbackFailed: return "AUX_POLL_FAIL";
        case SensorFaultCode::MagReadFailed: return "MAG_READ";
        case SensorFaultCode::BaroReadFailed: return "BARO_READ";
        default: return "UNKNOWN";
    }
}

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

    // Consume transition events first so an RC recovery can close the latched
    // loss episode before the current preflight snapshot is evaluated.
    reportSystemEvents();

    if (!preflight.canArm) {
        const bool rcSignalInvalid = preflight.firstFailureReason &&
            strcmp(preflight.firstFailureReason, "RC signal invalid") == 0;
        if (rcSignalInvalid) {
            if (!_rcSignalInvalidLatched) {
                _rcSignalInvalidLatched = true;
                _context.mavlink->sendStatusText("RC signal invalid", MAV_SEVERITY_WARNING);
            }
        } else {
            sendStatusTextThrottled(preflight.firstFailureReason);
        }
    } else if (_rcSignalInvalidLatched) {
        // RC can be disabled in setup, so a clean preflight also ends the old episode.
        _rcSignalInvalidLatched = false;
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

    const SensorFaultCode sensorFault = _context.sensors->getFaultCode();
    if (sensorFault != SensorFaultCode::None) {
        char faultText[50] = {};
        snprintf(faultText, sizeof(faultText), "SENSOR_FAULT %s", sensorFaultToken(sensorFault));
        sendStatusTextThrottled(faultText);
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

void RuntimeHealthReporter::reportSystemEvents() {
    SystemEvent event = {};
    while (_context.events->consume(event)) {
        char text[50] = {};
        if (event.type == SystemEventType::FailsafeEntered) {
            snprintf(text, sizeof(text), "FAILSAFE_ENTER %s MASK=0x%02lX",
                     FailsafeManager::reasonToken((uint16_t)event.detail),
                     (unsigned long)event.detail);
            _context.mavlink->sendStatusText(text, MAV_SEVERITY_CRITICAL);
        } else if (event.type == SystemEventType::FailsafeCleared) {
            _context.mavlink->sendStatusText("FAILSAFE_CLEAR", MAV_SEVERITY_INFO);
        } else if (event.type == SystemEventType::RcLost) {
            if (!_rcSignalInvalidLatched) {
                _rcSignalInvalidLatched = true;
                _context.mavlink->sendStatusText("RC_SIGNAL_LOST", MAV_SEVERITY_WARNING);
            }
        } else if (event.type == SystemEventType::RcRecovered) {
            if (_rcSignalInvalidLatched) {
                _rcSignalInvalidLatched = false;
                _context.mavlink->sendStatusText("RC_SIGNAL_OK", MAV_SEVERITY_INFO);
            }
        }
    }
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
