#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Arduino.h>
#include "../drivers/PioUart.h"
#include "../types.h"
#include "../core/state/ThreadSafeRingBuffer.h"

enum class BlackboxRecordType : uint8_t {
    Flight = 1,
    Timing = 2,
    RuntimeHealth = 3
};

struct __attribute__((packed)) BlackboxRecordHeader {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t payloadSize;
    uint32_t sequence;
    uint32_t timestampMs;
};

struct __attribute__((packed)) BlackboxFlightPayload {
    float roll, pitch, yaw;
    float gx, gy, gz;
    uint16_t throttle, aileron, elevator, rudder;
    uint8_t failsafe;
    uint8_t sensorHealth;
};

class Blackbox {
  public:
    static constexpr size_t MAX_PAYLOAD_SIZE = sizeof(TimingBudgetStatus) > sizeof(RuntimeHealthStatus)
        ? sizeof(TimingBudgetStatus) : sizeof(RuntimeHealthStatus);
    static constexpr size_t MAX_FRAME_SIZE = sizeof(BlackboxRecordHeader) + MAX_PAYLOAD_SIZE + sizeof(uint16_t);
    static constexpr uint8_t QUEUE_SLOTS = 8;

    void init();
    void setLogRateHz(uint8_t hz);
    void log(float roll, float pitch, float yaw,
             float gx, float gy, float gz,
             uint16_t throttle, uint16_t aileron,
             uint16_t elevator, uint16_t rudder,
             bool failsafe,
             SensorHealth sensorHealth);
    void logTimingBudget(const TimingBudgetStatus& status);
    void logRuntimeHealth(const RuntimeHealthStatus& status);
    uint8_t drain(uint8_t maxRecords = QUEUE_SLOTS);
    uint32_t droppedRecords() const { return _droppedRecords; }
    uint32_t backpressureRecords() const { return _backpressureRecords; }
    uint8_t queuedRecords() const { return _queue.pending(); }
    static uint16_t crc16(const uint8_t* data, size_t length);

  private:
    struct Frame {
        uint16_t size = 0;
        uint8_t bytes[MAX_FRAME_SIZE] = {};
    };

    bool _enabled = false;
    uint16_t _logPeriodMs = 200;
    uint32_t _lastLogMs = 0;
    uint32_t _sequence = 0;
    uint32_t _droppedRecords = 0;
    uint32_t _backpressureRecords = 0;
    uint8_t _backpressureDrainStreak = 0;
    bool _transportBackpressured = false;
    ThreadSafeRingBuffer<Frame, QUEUE_SLOTS> _queue;

    bool writeRecord(BlackboxRecordType type, const void* payload, uint16_t payloadSize);
};

extern Blackbox blackbox;

#endif
