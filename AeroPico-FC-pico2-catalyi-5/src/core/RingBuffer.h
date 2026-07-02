#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <Arduino.h>
#include "pico/platform.h"

// Lock-free Single Producer Single Consumer (SPSC) Ring Buffer
// Core 0 yazar (producer), Core 1 okur (consumer)
// Mutex gerektirmez — atomic index değişimi ile senkronizasyon

template<typename T, uint8_t SIZE>
class RingBuffer {
  public:
    RingBuffer() : _head(0), _tail(0) {}

    // Producer (Core 0) çağırır
    bool __not_in_flash_func(push)(const T& item) {
        uint8_t next_head = (_head + 1) % SIZE;
        if (next_head == _tail) return false;  // Dolu
        _buf[_head] = item;
        __dmb();  // Memory barrier — yazma tamamlanmadan index güncellenmez
        _head = next_head;
        return true;
    }

    // Consumer (Core 1) çağırır
    bool __not_in_flash_func(pop)(T& item) {
        if (_head == _tail) return false;  // Boş
        item = _buf[_tail];
        __dmb();  // Memory barrier — okuma tamamlanmadan index güncellenmez
        _tail = (_tail + 1) % SIZE;
        return true;
    }

    // En son değeri kopyasız oku (peek) — sadece okur, tüketmez
    bool __not_in_flash_func(peek)(T& item) const {
        if (_head == _tail) return false;
        uint8_t last = (_head == 0) ? SIZE - 1 : _head - 1;
        item = _buf[last];
        return true;
    }

    bool isEmpty() const { return _head == _tail; }
    bool isFull()  const { return ((_head + 1) % SIZE) == _tail; }

  private:
    volatile uint8_t _head;
    volatile uint8_t _tail;
    T _buf[SIZE];
};

#endif