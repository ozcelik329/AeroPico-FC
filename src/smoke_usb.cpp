#include <Arduino.h>
#include "tusb.h"

static void rawUsbWrite(const char* text) {
    if (tud_cdc_write_available() == 0) {
        tud_task();
    }
    tud_cdc_write_str(text);
    tud_cdc_write_flush();
    tud_task();
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    Serial.ignoreFlowControl(true);

    const uint32_t startMs = millis();
    while (!Serial && (millis() - startMs) < 4000) {
        digitalWrite(LED_BUILTIN, (millis() / 100) & 1 ? HIGH : LOW);
        delay(10);
    }

    Serial.println();
    Serial.println("[AEROPICO] USB smoke standalone");
    Serial.println("[AEROPICO] FreeRTOS disabled; no flight firmware linked");
    Serial.flush();
    rawUsbWrite("[AEROPICO] raw TinyUSB CDC path online\r\n");
}

void loop() {
    static uint32_t lastPrintMs = 0;
    static uint32_t lastBlinkMs = 0;
    static bool led = false;
    const uint32_t nowMs = millis();

    if (nowMs - lastBlinkMs >= 250) {
        lastBlinkMs = nowMs;
        led = !led;
        digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    }

    if (nowMs - lastPrintMs >= 1000) {
        lastPrintMs = nowMs;
        Serial.printf("[AEROPICO] standalone alive %lu ms\n", (unsigned long)nowMs);
        Serial.flush();
        char line[80];
        snprintf(line, sizeof(line), "[AEROPICO] raw alive %lu ms\r\n", (unsigned long)nowMs);
        rawUsbWrite(line);
    }

    delay(10);
}
