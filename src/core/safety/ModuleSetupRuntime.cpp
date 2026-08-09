#include "ModuleSetupRuntime.h"
#include "board/Config.h"

void ModuleSetupRuntime::update(const ModuleSetupSnapshot& snapshot) {
    _snapshot = snapshot;
}

bool ModuleSetupRuntime::batteryRequired() const {
    return _snapshot.batteryEnabled;
}

bool ModuleSetupRuntime::rcRequired() const {
    return _snapshot.rcEnabled && _snapshot.rcType != 0;
}

uint16_t ModuleSetupRuntime::enabledSensorMask(uint16_t detectedMask) const {
    uint16_t mask = detectedMask;
    if (!_snapshot.baroEnabled) mask &= ~(uint16_t)SENSOR_CAP_BARO;
    if (!_snapshot.magEnabled) mask &= ~(uint16_t)SENSOR_CAP_MAG;
    if (!_snapshot.gpsEnabled) mask &= ~(uint16_t)SENSOR_CAP_GPS;
    return mask;
}

ModuleSetupEvaluation ModuleSetupRuntime::evaluate(uint16_t detectedMask) const {
    ModuleSetupEvaluation result = {};
    result.enabledCapabilityMask = enabledSensorMask(detectedMask);

    if (!_snapshot.bootServoPinConfigValid || !validateServoPinSetup(_snapshot.servoPins)) {
        strncpy(result.reason, "Setup servo pin map invalid", sizeof(result.reason) - 1);
        return result;
    }

    if (!isSupportedImuType(_snapshot.imuType) ||
        !isSupportedBaroType(_snapshot.baroType) ||
        !isSupportedMagType(_snapshot.magType) ||
        !isSupportedGpsType(_snapshot.gpsType) ||
        !isSupportedRcType(_snapshot.rcType) ||
        !isSupportedBatteryType(_snapshot.batteryType)) {
        strncpy(result.reason, "Setup contains unsupported module type", sizeof(result.reason) - 1);
        return result;
    }

    if (_snapshot.batteryEnabled && _snapshot.batteryType == 0) {
        strncpy(result.reason, "Setup enables battery but type is None", sizeof(result.reason) - 1);
        return result;
    }

    if (_snapshot.baroEnabled && !hasSensorCapability(detectedMask, SENSOR_CAP_BARO)) {
        strncpy(result.reason, "Setup requires BARO but barometer is missing", sizeof(result.reason) - 1);
        return result;
    }

    if (_snapshot.magEnabled && !hasSensorCapability(detectedMask, SENSOR_CAP_MAG)) {
        strncpy(result.reason, "Setup requires MAG but magnetometer is missing", sizeof(result.reason) - 1);
        return result;
    }

    if (_snapshot.gpsEnabled && !hasSensorCapability(detectedMask, SENSOR_CAP_GPS)) {
        strncpy(result.reason, "Setup requires GPS but no valid fix is available", sizeof(result.reason) - 1);
        return result;
    }

    result.passed = true;
    result.batteryRequired = batteryRequired();
    result.rcRequired = rcRequired();
    snprintf(result.reason,
             sizeof(result.reason),
             "Module setup OK BARO=%u MAG=%u GPS=%u BATT=%u RC=%u",
             _snapshot.baroEnabled ? 1u : 0u,
             _snapshot.magEnabled ? 1u : 0u,
             _snapshot.gpsEnabled ? 1u : 0u,
             _snapshot.batteryEnabled ? 1u : 0u,
             _snapshot.rcEnabled ? 1u : 0u);
    return result;
}

bool ModuleSetupRuntime::validateServoPinSetup(const ModuleServoPinConfig& pins) {
    if (!isValidServoSetupPin(pins.aileron) ||
        !isValidServoSetupPin(pins.elevator) ||
        !isValidServoSetupPin(pins.rudder) ||
        !isValidServoSetupPin(pins.throttle)) {
        return false;
    }

    return pins.aileron != pins.elevator &&
           pins.aileron != pins.rudder &&
           pins.aileron != pins.throttle &&
           pins.elevator != pins.rudder &&
           pins.elevator != pins.throttle &&
           pins.rudder != pins.throttle;
}

bool ModuleSetupRuntime::isReservedServoSetupPin(uint8_t pin) {
    return pin == PIN_SBUS_RX ||
           pin == PIN_SDA ||
           pin == PIN_SCL ||
           pin == PIN_BENCH_ADMIN_GND ||
           pin == PIN_BENCH_ADMIN_SENSE;
}

bool ModuleSetupRuntime::isValidServoSetupPin(uint8_t pin) {
    return pin <= 28 && !isReservedServoSetupPin(pin);
}

bool ModuleSetupRuntime::isSupportedImuType(uint8_t type) { return type <= 1; }
bool ModuleSetupRuntime::isSupportedBaroType(uint8_t type) { return type <= 1; }
bool ModuleSetupRuntime::isSupportedMagType(uint8_t type) { return type <= 2; }
bool ModuleSetupRuntime::isSupportedGpsType(uint8_t type) { return type <= 1; }
bool ModuleSetupRuntime::isSupportedRcType(uint8_t type) { return type <= 1; }
bool ModuleSetupRuntime::isSupportedBatteryType(uint8_t type) { return type <= 1; }
