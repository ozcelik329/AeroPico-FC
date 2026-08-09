#include "PreflightRuntime.h"
#include "../sensors/SensorPreflightEvaluator.h"
#include "../../drivers/Sensors.h"
#include "../../drivers/RX.h"
#include "../../drivers/gps/GpsManager.h"

static constexpr uint32_t PREFLIGHT_MIN_FREE_HEAP_BYTES = 24 * 1024;

void PreflightRuntime::init(const PreflightRuntimeContext& context) {
    _context = context;
}

void PreflightRuntime::setMinSensorQuality(uint8_t minQuality) {
    _minSensorQuality = minQuality > 100 ? 100 : minQuality;
}

PreflightResult PreflightRuntime::evaluate(uint32_t freeHeapBytes, bool outputsReady, bool timingHealthy) {
    if (!_context.sensors || !_context.gps || !_context.receiver ||
        !_context.battery || !_context.moduleSetup || !_context.health) {
        return {false, "preflight runtime not initialized", 0};
    }

    BatteryStatus battery = _context.battery->evaluate();
    const bool sensorOk = evaluateSensors();
    const bool moduleSetupOk = evaluateModuleSetup();
    const bool batteryRequired = _context.moduleSetup->batteryRequired();
    const bool rcRequired = _context.moduleSetup->rcRequired();
    const bool batteryOk = !batteryRequired || (battery.configured && battery.healthy);
    const bool rcOk = _context.receiver->isValid() && !_context.receiver->isFailsafe();
    _latestBatteryCritical = batteryRequired && battery.configured && battery.brownout;

    _context.health->reset();
    _context.health->setCheck(PreflightCheckId::Boot, true, true, "");
    _context.health->setCheck(PreflightCheckId::Sensor, true, sensorOk, _sensorReason);
    _context.health->setCheck(PreflightCheckId::RC,
                              rcRequired,
                              rcOk,
                              rcRequired ? "RC signal invalid" : "RC disabled in setup");
    _context.health->setCheck(PreflightCheckId::ModuleSetup, true, moduleSetupOk, _moduleSetupReason);
    _context.health->setCheck(PreflightCheckId::Battery,
                              batteryRequired,
                              batteryOk,
                              batteryRequired ? battery.reason : "Battery disabled in setup");
    _context.health->setCheck(PreflightCheckId::Memory,
                              true,
                              freeHeapBytes >= PREFLIGHT_MIN_FREE_HEAP_BYTES,
                              "Free heap too low");
    _context.health->setCheck(PreflightCheckId::Actuator, true, outputsReady, "Actuator output not ready");
    _context.health->setCheck(PreflightCheckId::Failsafe,
                              rcRequired,
                              !_context.receiver->isFailsafe(),
                              rcRequired ? "RC failsafe active" : "RC disabled in setup");
    _context.health->setCheck(PreflightCheckId::Scheduler, true, timingHealthy, "Timing budget exceeded");
    _context.health->setCheck(PreflightCheckId::GPS, false, false, "GPS not configured");
    return _context.health->evaluate();
}

bool PreflightRuntime::evaluateSensors() {
    SensorBuffer latest = _context.sensors->getLatest();
    const SensorPreflightStatus status = SensorPreflightEvaluator::evaluate(
        _context.sensors->isImuAvailable(),
        latest,
        _minSensorQuality
    );
    SensorPreflightEvaluator::formatReason(status, _sensorReason, sizeof(_sensorReason));
    return status.passed;
}

bool PreflightRuntime::evaluateModuleSetup() {
    const SensorCapabilityStatus sensorCaps = _context.sensors->capabilities();
    const SensorCapabilityStatus gpsCaps = _context.gps->capabilities();
    const uint16_t detectedMask = sensorCaps.functionMask | gpsCaps.functionMask;
    const ModuleSetupEvaluation result = _context.moduleSetup->evaluate(detectedMask);
    strncpy(_moduleSetupReason, result.reason, sizeof(_moduleSetupReason) - 1);
    _moduleSetupReason[sizeof(_moduleSetupReason) - 1] = '\0';
    return result.passed;
}
