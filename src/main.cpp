#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "core/FlightManager.h"
#include "core/SystemTimer.h"
#include "utils/Logger.h"

FlightManager flightManager;

void taskSensor(void* pvParameters) {
    for (;;) {
        flightManager.update();
        watchdog_update();  // Watchdog'u besle
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void taskFlight(void* pvParameters) {
    SystemTimer::init();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);

    // Watchdog: 2 saniye içinde beslenmezse reset
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    Logger::init();
    flightManager.init();
    Logger::log("AeroPico FC Baslatildi!");

    // Watchdog'un son reset sebebini logla
    if (watchdog_caused_reboot()) {
        Logger::log("[WATCHDOG] Onceki oturum watchdog ile resetlendi!");
    }

    xTaskCreateAffinitySet(
        taskSensor, "SensorTask",
        2048, NULL, 2,
        (1 << 0),   // affinity mask
        NULL        // task handle
    );

    xTaskCreateAffinitySet(
        taskFlight, "FlightTask",
        2048, NULL, 3,
        (1 << 1),   // affinity mask
        NULL        // task handle
    );

    vTaskStartScheduler();
}

void loop() {}