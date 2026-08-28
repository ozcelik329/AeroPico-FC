#include "UsbCdcTx.h"

#include <algorithm>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

uint8_t UsbCdcTx::_queue[UsbCdcTx::QUEUE_CAPACITY] = {};
size_t UsbCdcTx::_head = 0;
size_t UsbCdcTx::_tail = 0;
size_t UsbCdcTx::_count = 0;
uint8_t UsbCdcTx::_lock = 0;
uint32_t UsbCdcTx::_droppedWrites = 0;

bool UsbCdcTx::tryLock() {
    return !__atomic_test_and_set(&_lock, __ATOMIC_ACQUIRE);
}

void UsbCdcTx::unlock() {
    __atomic_clear(&_lock, __ATOMIC_RELEASE);
}

void UsbCdcTx::recordDrop() {
    __atomic_add_fetch(&_droppedWrites, 1u, __ATOMIC_RELAXED);
}

bool UsbCdcTx::enqueueLocked(const uint8_t* data, size_t len) {
    if (len > QUEUE_CAPACITY - _count) {
        return false;
    }

    const size_t first = std::min(len, QUEUE_CAPACITY - _head);
    memcpy(&_queue[_head], data, first);
    if (len > first) {
        memcpy(_queue, data + first, len - first);
    }
    _head = (_head + len) % QUEUE_CAPACITY;
    _count += len;
    return true;
}

bool UsbCdcTx::enqueue(const uint8_t* data, size_t len) {
    if (!data || len == 0 || len > QUEUE_CAPACITY || !Serial) {
        recordDrop();
        return false;
    }
    if (!tryLock()) {
        recordDrop();
        return false;
    }

    const bool queued = enqueueLocked(data, len);
    unlock();
    if (!queued) {
        recordDrop();
    }
    return queued;
}

bool UsbCdcTx::enqueueText(const char* text) {
    if (!text) {
        recordDrop();
        return false;
    }
    return enqueue(reinterpret_cast<const uint8_t*>(text), strlen(text));
}

bool UsbCdcTx::enqueueLine(const char* text) {
    if (!text) {
        recordDrop();
        return false;
    }

    const size_t len = strlen(text);
    if (len + 1 > FORMAT_CAPACITY) {
        recordDrop();
        return false;
    }
    char buffer[FORMAT_CAPACITY];
    memcpy(buffer, text, len);
    buffer[len] = '\n';
    return enqueue(reinterpret_cast<const uint8_t*>(buffer), len + 1);
}

bool UsbCdcTx::enqueueFormat(const char* format, ...) {
    if (!format) {
        recordDrop();
        return false;
    }

    char buffer[FORMAT_CAPACITY];
    va_list args;
    va_start(args, format);
    const int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (len <= 0 || static_cast<size_t>(len) >= sizeof(buffer)) {
        recordDrop();
        return false;
    }
    return enqueue(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(len));
}

void UsbCdcTx::service() {
    if (!Serial || !tryLock()) {
        return;
    }

    const int available = Serial.availableForWrite();
    if (available <= 0 || _count == 0) {
        unlock();
        return;
    }

    const size_t chunk = std::min(
        std::min(_count, static_cast<size_t>(available)),
        QUEUE_CAPACITY - _tail
    );
    const size_t written = Serial.write(&_queue[_tail], chunk);
    if (written > 0) {
        _tail = (_tail + written) % QUEUE_CAPACITY;
        _count -= written;
    }
    unlock();
}

size_t UsbCdcTx::queuedBytes() {
    if (!tryLock()) {
        return 0;
    }
    const size_t count = _count;
    unlock();
    return count;
}

uint32_t UsbCdcTx::droppedWrites() {
    return __atomic_load_n(&_droppedWrites, __ATOMIC_RELAXED);
}

#ifdef UNIT_TEST
void UsbCdcTx::resetForTest() {
    while (!tryLock()) {
    }
    _head = 0;
    _tail = 0;
    _count = 0;
    _droppedWrites = 0;
    unlock();
}
#endif
