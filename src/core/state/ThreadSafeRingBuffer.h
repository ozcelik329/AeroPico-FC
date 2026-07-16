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
        ScopedLock g(*this);
        uint8_t next = nextIndex(_head);
        if (next == _tail) return false;
        _buf[_head] = item;
        _head = next;
        return true;
    }

    bool pop(T& item) {
        ScopedLock g(*this);
        if (_head == _tail) return false;
        item = _buf[_tail];
        _tail = nextIndex(_tail);
        return true;
    }

    bool peek(T& item) const {
        ScopedLock g(*this);
        if (_head == _tail) return false;
        item = _buf[_tail];
        return true;
    }

    bool isEmpty() const { ScopedLock g(*this); return _head == _tail; }
    bool isFull()  const { ScopedLock g(*this); return nextIndex(_head) == _tail; }
    void reset() {
        ScopedLock g(*this);
        _head = 0;
        _tail = 0;
    }
    uint8_t pending() const {
        ScopedLock g(*this);
        return usedLocked();
    }
    static constexpr uint8_t capacity() { return SIZE - 1; }

  private:
    class ScopedLock {
      public:
        explicit ScopedLock(const ThreadSafeRingBuffer& owner) : _owner(owner) { _owner._mtx.lock(); }
        ~ScopedLock() { _owner._mtx.unlock(); }
      private:
        const ThreadSafeRingBuffer& _owner;
    };

    static constexpr bool isPowerOfTwo() { return (SIZE & (SIZE - 1u)) == 0; }
    static inline uint8_t nextIndex(uint8_t index) {
        return isPowerOfTwo()
            ? (uint8_t)((index + 1u) & (SIZE - 1u))
            : (uint8_t)((index + 1u) % SIZE);
    }
    uint8_t usedLocked() const { return (uint8_t)((_head + SIZE - _tail) % SIZE); }

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
        ScopedLock g(*this);
        uint8_t next = nextIndex(_head);
        if (next == _tail) return false;
        _buf[_head] = item;
        _head = next;
        return true;
    }

    bool pop(T& item) {
        ScopedLock g(*this);
        if (_head == _tail) return false;
        item = _buf[_tail];
        _tail = nextIndex(_tail);
        return true;
    }

    bool peek(T& item) const {
        ScopedLock g(*this);
        if (_head == _tail) return false;
        item = _buf[_tail];
        return true;
    }

    bool isEmpty() const { ScopedLock g(*this); return _head == _tail; }
    bool isFull()  const { ScopedLock g(*this); return nextIndex(_head) == _tail; }
    void reset() {
        ScopedLock g(*this);
        _head = 0;
        _tail = 0;
    }
    uint8_t pending() const {
        ScopedLock g(*this);
        return usedLocked();
    }
    static constexpr uint8_t capacity() { return SIZE - 1; }

  private:
    class ScopedLock {
      public:
        explicit ScopedLock(const ThreadSafeRingBuffer& owner) : _owner(owner) { mutex_enter_blocking(&_owner._mutex); }
        ~ScopedLock() { mutex_exit(&_owner._mutex); }
      private:
        const ThreadSafeRingBuffer& _owner;
    };

    static constexpr bool isPowerOfTwo() { return (SIZE & (SIZE - 1u)) == 0; }
    static inline uint8_t nextIndex(uint8_t index) {
        return isPowerOfTwo()
            ? (uint8_t)((index + 1u) & (SIZE - 1u))
            : (uint8_t)((index + 1u) % SIZE);
    }
    uint8_t usedLocked() const { return (uint8_t)((_head + SIZE - _tail) % SIZE); }

    volatile uint8_t _head;
    volatile uint8_t _tail;
    T _buf[SIZE] = {};
    mutable mutex_t _mutex;
};
#endif

#endif
