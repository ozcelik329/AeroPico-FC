#include "BootLogger.h"
#include "../BuildInfo.h"
#include "../def.h"

void BootLogger::printBanner() {
    Serial.println("==============================");
    Serial.printf(" %s %s\n", AEROPICO_FIRMWARE_NAME, AEROPICO_VERSION);
    Serial.printf(" Target : %s\n", AEROPICO_TARGET);
    Serial.printf(" CPU    : %s\n", AEROPICO_MCU);
    Serial.println("==============================");
}

void BootLogger::printReadyMessage() {
    Serial.println();
    Serial.println("System Ready.");
    Serial.printf("Loop Frequency: %luHz\n", 1000000UL / FLIGHT_LOOP_PERIOD_US);
}

void BootLogger::printHealthReport(
    uint32_t loopRateHz,
    bool imuOk,
    bool baroOk,
    bool magOk,
    bool receiverOk,
    bool dmaOk,
    bool armed,
    bool failsafe,
    uint32_t heapBytes
) {
    Serial.println();
    Serial.println("========================");
    Serial.println("SYSTEM HEALTH");
    Serial.println("========================");
    Serial.printf("CPU Temp........%s\n", "N/A");
    Serial.printf("Loop Rate.......%lu Hz\n", loopRateHz);
    Serial.printf("IMU..............%s\n", imuOk ? "OK" : "FAIL");
    Serial.printf("Barometer........%s\n", baroOk ? "OK" : "FAIL");
    Serial.printf("Compass..........%s\n", magOk ? "OK" : "FAIL");
    Serial.printf("Receiver.........%s\n", receiverOk ? "OK" : "FAIL");
    Serial.printf("GPS..............Not Connected\n");
    Serial.printf("DMA..............%s\n", dmaOk ? "OK" : "FAIL");
    Serial.printf("Heap.............%lu KB\n", heapBytes / 1024);
    Serial.printf("Failsafe.........%s\n", failsafe ? "ON" : "OFF");
    Serial.printf("Armed............%s\n", armed ? "YES" : "NO");
    Serial.println("Flight Mode......MANUAL");
}

void BootLogger::ok(const char* name) {
    Serial.printf("[BOOT] %-18s OK\n", name);
}

void BootLogger::okWithValue(const char* name, const char* value) {
    Serial.printf("[BOOT] %-18s OK (%s)\n", name, value);
}

void BootLogger::fail(const char* name, const char* reason) {
    Serial.printf("[BOOT] %-18s FAILED\n", name);
    Serial.printf("       %s\n", reason);
}

void BootLogger::warn(const char* name, const char* reason) {
    Serial.printf("[BOOT] %-18s WARN\n", name);
    Serial.printf("       %s\n", reason);
}

void BootLogger::info(const char* text) {
    Serial.println(text);
}
