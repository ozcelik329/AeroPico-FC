#include "Blackbox.h"

Blackbox blackbox;

void Blackbox::init() {
    _enabled = true;
    // ESP32'ye başlangıç sinyali gönder
    const char* header = "BB_START\n";
    espUart.write((const uint8_t*)header, strlen(header));
    Serial.println("[BLACKBOX] Baslatildi.");
}

void Blackbox::log(float roll, float pitch, float yaw,
                   float gx, float gy, float gz,
                   uint16_t throttle, uint16_t aileron,
                   uint16_t elevator, uint16_t rudder,
                   bool failsafe) {
    if (!_enabled) return;

    char buf[128];
    snprintf(buf, sizeof(buf),
        "BB,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%u,%u,%u,%d\n",
        (unsigned long)millis(),
        roll, pitch, yaw,
        gx, gy, gz,
        throttle, aileron, elevator, rudder,
        failsafe ? 1 : 0
    );

    espUart.write((const uint8_t*)buf, strlen(buf));
}