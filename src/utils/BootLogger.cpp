#include "BootLogger.h"
#include "UsbCdcTx.h"
#include "../BuildInfo.h"
#include "../def.h"

void BootLogger::printBanner() {
    UsbCdcTx::enqueueLine("==============================");
    UsbCdcTx::enqueueFormat(" %s %s\n", AEROPICO_FIRMWARE_NAME, AEROPICO_VERSION);
    UsbCdcTx::enqueueFormat(" Target : %s\n", AEROPICO_TARGET);
    UsbCdcTx::enqueueFormat(" CPU    : %s\n", AEROPICO_MCU);
    UsbCdcTx::enqueueLine("==============================");
}

void BootLogger::printReadyMessage() {
    UsbCdcTx::enqueueLine("");
    UsbCdcTx::enqueueLine("System Ready.");
    UsbCdcTx::enqueueFormat("Loop Frequency: %luHz\n", 1000000UL / FLIGHT_LOOP_PERIOD_US);
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
    UsbCdcTx::enqueueLine("");
    UsbCdcTx::enqueueLine("========================");
    UsbCdcTx::enqueueLine("SYSTEM HEALTH");
    UsbCdcTx::enqueueLine("========================");
    UsbCdcTx::enqueueFormat("CPU Temp........%s\n", "N/A");
    UsbCdcTx::enqueueFormat("Loop Rate.......%lu Hz\n", loopRateHz);
    UsbCdcTx::enqueueFormat("IMU..............%s\n", imuOk ? "OK" : "FAIL");
    UsbCdcTx::enqueueFormat("Barometer........%s\n", baroOk ? "OK" : "FAIL");
    UsbCdcTx::enqueueFormat("Compass..........%s\n", magOk ? "OK" : "FAIL");
    UsbCdcTx::enqueueFormat("Receiver.........%s\n", receiverOk ? "OK" : "FAIL");
    UsbCdcTx::enqueueLine("GPS..............Not Connected");
    UsbCdcTx::enqueueFormat("DMA..............%s\n", dmaOk ? "OK" : "FAIL");
    UsbCdcTx::enqueueFormat("Heap.............%lu KB\n", heapBytes / 1024);
    UsbCdcTx::enqueueFormat("Failsafe.........%s\n", failsafe ? "ON" : "OFF");
    UsbCdcTx::enqueueFormat("Armed............%s\n", armed ? "YES" : "NO");
    UsbCdcTx::enqueueLine("Flight Mode......MANUAL");
}

void BootLogger::ok(const char* name) {
    UsbCdcTx::enqueueFormat("[BOOT] %-18s OK\n", name);
}

void BootLogger::okWithValue(const char* name, const char* value) {
    UsbCdcTx::enqueueFormat("[BOOT] %-18s OK (%s)\n", name, value);
}

void BootLogger::fail(const char* name, const char* reason) {
    UsbCdcTx::enqueueFormat("[BOOT] %-18s FAILED\n", name);
    UsbCdcTx::enqueueFormat("       %s\n", reason);
}

void BootLogger::warn(const char* name, const char* reason) {
    UsbCdcTx::enqueueFormat("[BOOT] %-18s WARN\n", name);
    UsbCdcTx::enqueueFormat("       %s\n", reason);
}

void BootLogger::info(const char* text) {
    UsbCdcTx::enqueueLine(text);
}
