#ifndef THREAD_SAFE_RING_BUFFER_H
#define THREAD_SAFE_RING_BUFFER_H

#include <Arduino.h>
// Support both Pico mutex and host std::mutex for UNIT_TEST (native) builds
#ifdef UNIT_TEST
#include <mutex>

template<typename T, uint8_t SIZE>
class ThreadSafeRingBuffer {
    static_assert(SIZE > 1, "ThreadSafeRingBuffer needs at least two slots");

  public:
    ThreadSafeRingBuffer() : _head(0), _tail(0) {}

    bool push(const T& item) {
        std::lock_guard<std::mutex> g(_mtx);
        uint8_t next = nextIndex(_head);
        if (next == _tail) return false;
        _buf[_head] = item;
        _head = next;
        return true;
    }

    bool pop(T& item) {
        std::lock_guard<std::mutex> g(_mtx);
        if (_head == _tail) return false;
        item = _buf[_tail];
        _tail = nextIndex(_tail);
        return true;
    }

    bool peek(T& item) const {
        std::lock_guard<std::mutex> g(_mtx);
        if (_head == _tail) return false;
        uint8_t last = (_head == 0) ? (SIZE - 1) : (_head - 1);
        item = _buf[last];
        return true;
    }

    bool isEmpty() const { std::lock_guard<std::mutex> g(_mtx); return _head == _tail; }
    bool isFull()  const { std::lock_guard<std::mutex> g(_mtx); return nextIndex(_head) == _tail; }
    void reset() {
        std::lock_guard<std::mutex> g(_mtx);
        _head = 0;
        _tail = 0;
    }
    uint8_t pending() const {
        std::lock_guard<std::mutex> g(_mtx);
        return (uint8_t)((_head + SIZE - _tail) % SIZE);
    }
    static constexpr uint8_t capacity() { return SIZE - 1; }

  private:
    static constexpr bool isPowerOfTwo() { return (SIZE & (SIZE - 1u)) == 0; }
    static inline uint8_t nextIndex(uint8_t index) {
        return isPowerOfTwo()
            ? (uint8_t)((index + 1u) & (SIZE - 1u))
            : (uint8_t)((index + 1u) % SIZE);
    }

    mutable std::mutex _mtx;
    volatile uint8_t _head = 0;
    volatile uint8_t _tail = 0;
    T _buf[SIZE] = {};
};
#else
#include <pico/mutex.h>

template<typename T, uint8_t SIZE>
class ThreadSafeRingBuffer {
    static_assert(SIZE > 1, "ThreadSafeRingBuffer needs at least two slots");

  public:
    ThreadSafeRingBuffer() : _head(0), _tail(0) { mutex_init(&_mutex); }

    bool push(const T& item) {
        mutex_enter_blocking(&_mutex);
        uint8_t next = nextIndex(_head);
        if (next == _tail) { mutex_exit(&_mutex); return false; }
        _buf[_head] = item;
        _head = next;
        mutex_exit(&_mutex);
        return true;
    }

    bool pop(T& item) {
        mutex_enter_blocking(&_mutex);
        if (_head == _tail) { mutex_exit(&_mutex); return false; }
        item = _buf[_tail];
        _tail = nextIndex(_tail);
        mutex_exit(&_mutex);
        return true;
    }

    bool peek(T& item) const {
        mutex_enter_blocking(&_mutex);
        if (_head == _tail) { mutex_exit(&_mutex); return false; }
        uint8_t last = (_head == 0) ? (SIZE - 1) : (_head - 1);
        item = _buf[last];
        mutex_exit(&_mutex);
        return true;
    }

    bool isEmpty() const { mutex_enter_blocking(&_mutex); bool e = (_head == _tail); mutex_exit(&_mutex); return e; }
    bool isFull()  const { mutex_enter_blocking(&_mutex); bool f = (nextIndex(_head) == _tail); mutex_exit(&_mutex); return f; }
    void reset() {
        mutex_enter_blocking(&_mutex);
        _head = 0;
        _tail = 0;
        mutex_exit(&_mutex);
    }
    uint8_t pending() const {
        mutex_enter_blocking(&_mutex);
        uint8_t count = (uint8_t)((_head + SIZE - _tail) % SIZE);
        mutex_exit(&_mutex);
        return count;
    }
    static constexpr uint8_t capacity() { return SIZE - 1; }

  private:
    static constexpr bool isPowerOfTwo() { return (SIZE & (SIZE - 1u)) == 0; }
    static inline uint8_t nextIndex(uint8_t index) {
        return isPowerOfTwo()
            ? (uint8_t)((index + 1u) & (SIZE - 1u))
            : (uint8_t)((index + 1u) % SIZE);
    }

    volatile uint8_t _head;
    volatile uint8_t _tail;
    T _buf[SIZE] = {};
    mutable mutex_t _mutex;
};
#endif

#endif
