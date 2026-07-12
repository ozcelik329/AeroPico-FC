#include "GpsManager.h"

void GpsManager::init(IHALUART* uart, bool enabled, uint32_t baud) {
    _uart = uart;
    _parser.reset();
    _status = {};
    _status.enabled = enabled;
    if (_status.enabled && _uart) {
        _uart->begin(baud);
    }
}

void GpsManager::update(uint32_t nowMs) {
    if (!_status.enabled || !_uart) return;

    while (_uart->available() > 0) {
        const int byte = _uart->read();
        if (byte < 0) break;
        GpsFix fix = {};
        if (_parser.push((char)byte, fix)) {
            _status.fix = fix;
            _status.lastFixMs = nowMs;
        }
    }
    _status.sentences = _parser.sentences();
    _status.checksumErrors = _parser.checksumErrors();
}

SensorCapabilityStatus GpsManager::capabilities() const {
    SensorCapabilityStatus capability = {};
    capability.gpsAvailable = isAvailable();
    if (capability.gpsAvailable) {
        capability.functionMask = SENSOR_CAP_GPS;
    }
    return capability;
}
