#include "Blackbox.h"

Blackbox blackbox;

void Blackbox::init() {
    _enabled = true;
    // ESP32'ye başlangıç sinyali gönder
    const char* header = "BB_START\n";
    espUart.write((const uint8_t*)header, strlen(header));
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

    char buf[128];
    snprintf(buf, sizeof(buf),
        "BB,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%u,%u,%u,%d,%u\n",
        (unsigned long)nowMs,
        roll, pitch, yaw,
        gx, gy, gz,
        throttle, aileron, elevator, rudder,
        failsafe ? 1 : 0,
        (unsigned)sensorHealth
    );

    espUart.write((const uint8_t*)buf, strlen(buf));
}

void Blackbox::logTimingBudget(const TimingBudgetStatus& status) {
    if (!_enabled) return;

    char buf[192];
    snprintf(buf, sizeof(buf),
        "BT,%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d\n",
        (unsigned long)millis(),
        (unsigned)status.consumeUs,
        (unsigned)status.pidUs,
        (unsigned)status.mixerUs,
        (unsigned)status.totalUs,
        (unsigned)status.consumeAvgUs,
        (unsigned)status.pidAvgUs,
        (unsigned)status.mixerAvgUs,
        (unsigned)status.totalAvgUs,
        (unsigned)status.consumeJitterUs,
        (unsigned)status.pidJitterUs,
        (unsigned)status.mixerJitterUs,
        (unsigned)status.totalJitterUs,
        (unsigned)status.consumeDeadlineMisses,
        (unsigned)status.pidDeadlineMisses,
        (unsigned)status.mixerDeadlineMisses,
        (unsigned)status.totalDeadlineMisses,
        status.consumeExceeded ? 1 : 0,
        status.pidExceeded ? 1 : 0,
        status.mixerExceeded ? 1 : 0,
        status.totalExceeded ? 1 : 0
    );

    espUart.write((const uint8_t*)buf, strlen(buf));
}
