#include "Blackbox.h"

#include <string.h>

Blackbox blackbox;

void Blackbox::init() {
    _enabled = true;
    _sequence = 0;
    _droppedRecords = 0;
    Serial.println("[BLACKBOX] Baslatildi.");
}

void Blackbox::setLogRateHz(uint8_t hz) {
    if (hz < 1) hz = 1;
    if (hz > 50) hz = 50;
    _logPeriodMs = (uint16_t)(1000u / hz);
}

void Blackbox::log(float roll, float pitch, float yaw,
                   float gx, float gy, float gz,
                   uint16_t throttle, uint16_t aileron,
                   uint16_t elevator, uint16_t rudder,
                   bool failsafe,
                   SensorHealth sensorHealth) {
    if (!_enabled) return;

    const uint32_t nowMs = millis();
    if (nowMs - _lastLogMs < _logPeriodMs) {
        return;
    }
    _lastLogMs = nowMs;

    BlackboxFlightPayload payload = {
        roll, pitch, yaw,
        gx, gy, gz,
        throttle, aileron, elevator, rudder,
        (uint8_t)(failsafe ? 1 : 0),
        (uint8_t)sensorHealth
    };
    writeRecord(BlackboxRecordType::Flight, &payload, sizeof(payload));
}

void Blackbox::logTimingBudget(const TimingBudgetStatus& status) {
    if (!_enabled) return;
    writeRecord(BlackboxRecordType::Timing, &status, sizeof(status));
}

void Blackbox::logRuntimeHealth(const RuntimeHealthStatus& status) {
    if (!_enabled) return;
    writeRecord(BlackboxRecordType::RuntimeHealth, &status, sizeof(status));
}

uint16_t Blackbox::crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

bool Blackbox::writeRecord(BlackboxRecordType type, const void* payload, uint16_t payloadSize) {
    constexpr uint16_t MAGIC = 0x4242;
    constexpr uint8_t VERSION = 1;
    constexpr size_t MAX_PAYLOAD = sizeof(TimingBudgetStatus) > sizeof(RuntimeHealthStatus)
        ? sizeof(TimingBudgetStatus) : sizeof(RuntimeHealthStatus);
    static_assert(sizeof(BlackboxFlightPayload) <= MAX_PAYLOAD, "Blackbox payload limit");

    if (!payload || payloadSize > MAX_PAYLOAD) {
        _droppedRecords++;
        return false;
    }

    uint8_t frame[sizeof(BlackboxRecordHeader) + MAX_PAYLOAD + sizeof(uint16_t)] = {};
    BlackboxRecordHeader header = {
        MAGIC, VERSION, (uint8_t)type, payloadSize, _sequence++, (uint32_t)millis()
    };
    memcpy(frame, &header, sizeof(header));
    memcpy(frame + sizeof(header), payload, payloadSize);
    const size_t bodySize = sizeof(header) + payloadSize;
    const uint16_t crc = crc16(frame, bodySize);
    memcpy(frame + bodySize, &crc, sizeof(crc));
    const size_t frameSize = bodySize + sizeof(crc);

    if (espUart.write(frame, frameSize) != frameSize) {
        _droppedRecords++;
        return false;
    }
    return true;
}
