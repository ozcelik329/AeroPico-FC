#include "BlackboxSdSink.h"

#if !defined(UNIT_TEST)
bool BlackboxSdSink::begin() {
    _ready = false;
    _bytesSinceFlush = 0;

    if (!SD.begin(_csPin, _spiHz)) {
        return false;
    }

    _file = SD.open(_path, FILE_WRITE);
    if (!_file) {
        return false;
    }

    _ready = true;
    return true;
}

size_t BlackboxSdSink::availableForWrite() const {
    return _ready ? 65535u : 0u;
}

size_t BlackboxSdSink::write(const uint8_t* data, size_t length) {
    if (!_ready || !data || length == 0) {
        return 0;
    }

    const size_t written = _file.write(data, length);
    _bytesSinceFlush = (uint16_t)(_bytesSinceFlush + written);
    if (_bytesSinceFlush >= FLUSH_BYTES) {
        _file.flush();
        _bytesSinceFlush = 0;
    }
    return written;
}
#endif
