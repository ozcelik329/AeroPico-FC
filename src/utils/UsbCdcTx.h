#ifndef USB_CDC_TX_H
#define USB_CDC_TX_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

class UsbCdcTx {
  public:
    static bool enqueue(const uint8_t* data, size_t len);
    static bool enqueueText(const char* text);
    static bool enqueueLine(const char* text);
    static bool enqueueFormat(const char* format, ...);
    static void service();
    static size_t queuedBytes();
    static uint32_t droppedWrites();

#ifdef UNIT_TEST
    static void resetForTest();
#endif

  private:
    static constexpr size_t QUEUE_CAPACITY = 2048;
    static constexpr size_t FORMAT_CAPACITY = 192;

    static bool tryLock();
    static void unlock();
    static bool enqueueLocked(const uint8_t* data, size_t len);
    static void recordDrop();

    static uint8_t _queue[QUEUE_CAPACITY];
    static size_t _head;
    static size_t _tail;
    static size_t _count;
    static uint8_t _lock;
    static uint32_t _droppedWrites;
};

#endif
