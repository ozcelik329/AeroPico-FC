#ifndef THREAD_SAFE_RING_BUFFER_H
#define THREAD_SAFE_RING_BUFFER_H

#include <Arduino.h>
// Support both Pico mutex and host std::mutex for UNIT_TEST (native) builds
#ifdef UNIT_TEST
#include <mutex>

template<typename T, uint8_t SIZE>
class ThreadSafeRingBuffer {
  public:
    ThreadSafeRingBuffer() : _head(0), _tail(0) {}

    bool push(const T& item) {
        std::lock_guard<std::mutex> g(_mtx);
        uint8_t next = (_head + 1) % SIZE;
        if (next == _tail) return false;
        _buf[_head] = item;
        _head = next;
        return true;
    }

    bool pop(T& item) {
        std::lock_guard<std::mutex> g(_mtx);
        if (_head == _tail) return false;
        item = _buf[_tail];
        _tail = (_tail + 1) % SIZE;
        return true;
    }

    bool peek(T& item) const {
        // const-correctness: cast away to lock
        ThreadSafeRingBuffer* self = (ThreadSafeRingBuffer*)this;
        std::lock_guard<std::mutex> g(self->_mtx);
        if (self->_head == self->_tail) return false;
        uint8_t last = (self->_head == 0) ? (SIZE - 1) : (self->_head - 1);
        item = self->_buf[last];
        return true;
    }

    bool isEmpty() const { std::lock_guard<std::mutex> g(_mtx); return _head == _tail; }
    bool isFull()  const { std::lock_guard<std::mutex> g(_mtx); return ((_head + 1) % SIZE) == _tail; }

  private:
    mutable std::mutex _mtx;
    volatile uint8_t _head = 0;
    volatile uint8_t _tail = 0;
    T _buf[SIZE];
};
#else
#include <pico/mutex.h>

template<typename T, uint8_t SIZE>
class ThreadSafeRingBuffer {
  public:
    ThreadSafeRingBuffer() : _head(0), _tail(0) { mutex_init(&_mutex); }

    bool push(const T& item) {
        mutex_enter_blocking(&_mutex);
        uint8_t next = (_head + 1) % SIZE;
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
        _tail = (_tail + 1) % SIZE;
        mutex_exit(&_mutex);
        return true;
    }

    bool peek(T& item) const {
        ThreadSafeRingBuffer* self = (ThreadSafeRingBuffer*)this;
        mutex_enter_blocking(&self->_mutex);
        if (self->_head == self->_tail) { mutex_exit(&self->_mutex); return false; }
        uint8_t last = (self->_head == 0) ? (SIZE - 1) : (self->_head - 1);
        item = self->_buf[last];
        mutex_exit(&self->_mutex);
        return true;
    }

    bool isEmpty() const { ThreadSafeRingBuffer* self = (ThreadSafeRingBuffer*)this; mutex_enter_blocking(&self->_mutex); bool e = (self->_head == self->_tail); mutex_exit(&self->_mutex); return e; }
    bool isFull()  const { ThreadSafeRingBuffer* self = (ThreadSafeRingBuffer*)this; mutex_enter_blocking(&self->_mutex); bool f = (((self->_head + 1) % SIZE) == self->_tail); mutex_exit(&self->_mutex); return f; }

  private:
    volatile uint8_t _head;
    volatile uint8_t _tail;
    T _buf[SIZE];
    mutex_t _mutex;
};
#endif

#endif
