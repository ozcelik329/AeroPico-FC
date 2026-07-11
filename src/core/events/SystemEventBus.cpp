#include "SystemEventBus.h"

SystemEventBus systemEventBus;

bool SystemEventBus::publish(const SystemEvent& event) {
    const uint32_t head = __atomic_load_n(&_head, __ATOMIC_RELAXED);
    const uint32_t next = (head + 1u) % CAPACITY;
    if (next == __atomic_load_n(&_tail, __ATOMIC_ACQUIRE)) {
        __atomic_add_fetch(&_dropped, 1u, __ATOMIC_RELAXED);
        return false;
    }
    _events[head] = event;
    __atomic_store_n(&_head, next, __ATOMIC_RELEASE);
    return true;
}

bool SystemEventBus::consume(SystemEvent& event) {
    const uint32_t tail = __atomic_load_n(&_tail, __ATOMIC_RELAXED);
    if (tail == __atomic_load_n(&_head, __ATOMIC_ACQUIRE)) {
        return false;
    }
    event = _events[tail];
    __atomic_store_n(&_tail, (tail + 1u) % CAPACITY, __ATOMIC_RELEASE);
    return true;
}

uint32_t SystemEventBus::droppedCount() const {
    return __atomic_load_n(&_dropped, __ATOMIC_ACQUIRE);
}

size_t SystemEventBus::pendingCount() const {
    const uint32_t head = __atomic_load_n(&_head, __ATOMIC_ACQUIRE);
    const uint32_t tail = __atomic_load_n(&_tail, __ATOMIC_ACQUIRE);
    return (head + CAPACITY - tail) % CAPACITY;
}

void SystemEventBus::reset() {
    __atomic_store_n(&_head, 0u, __ATOMIC_RELEASE);
    __atomic_store_n(&_tail, 0u, __ATOMIC_RELEASE);
    __atomic_store_n(&_dropped, 0u, __ATOMIC_RELEASE);
}
