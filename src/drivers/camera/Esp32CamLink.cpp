#include "Esp32CamLink.h"

void Esp32CamLink::init(IHALUART* uart, bool enabled, uint32_t baud) {
    _uart = uart;
    _status = {};
    _status.enabled = enabled;
    if (_status.enabled && _uart) {
        _uart->begin(baud);
    }
}

void Esp32CamLink::update(uint32_t nowMs) {
    if (!_status.enabled || !_uart) return;

    while (_uart->available() > 0) {
        const int byte = _uart->read();
        if (byte < 0) break;
        (void)byte;
        _status.bytesRx++;
        _status.lastByteMs = nowMs;
        _status.linkActive = true;
    }

    if (_status.linkActive && (nowMs - _status.lastByteMs > LINK_TIMEOUT_MS)) {
        _status.linkActive = false;
    }
}
