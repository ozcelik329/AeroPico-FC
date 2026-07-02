#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "core/FlightManager.h"
#include "core/SystemTimer.h"
#include "utils/Logger.h"
#include "telemetry/MavlinkHandler.h"
#include "telemetry/Blackbox.h"

FlightManager flightManager;

// Concrete drivers
#include "drivers/Sensors.h"
#include "drivers/RX.h"

SensorManager sensorManager;
RXManager rxManager;

void taskSensor(void* pvParameters) {
    for (;;) {
        flightManager.update();
        watchdog_update();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void taskFlight(void* pvParameters) {
    SystemTimer::init();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskTelemetry(void* pvParameters) {
    for (;;) {
        mavlink.update();  // Tüm stream'ler burada yönetiliyor

        // Blackbox: 50 Hz — bağımsız olarak devam eder
        FlightData d;
        if (flightManager.peekLatest(d)) {
            blackbox.log(
                d.roll,
                d.pitch,
                d.yaw,
                d.gyroX,
                d.gyroY,
                d.gyroZ,
                d.throttle,
                d.aileron,
                d.elevator,
                d.rudder,
                false
            );
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz
    }
}

void setup() {
    Serial.begin(115200);

    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    Logger::init();
    flightManager.init(&sensorManager, &rxManager);
    mavlink.init();
    blackbox.init();

    if (watchdog_caused_reboot()) {
        Logger::log("[WATCHDOG] Onceki oturum watchdog ile resetlendi!");
    }

    Logger::log("AeroPico FC Baslatildi!");

    xTaskCreateAffinitySet(
        taskSensor, "SensorTask",
        2048, NULL, 2,
        (1 << 0), NULL
    );

    xTaskCreateAffinitySet(
        taskFlight, "FlightTask",
        2048, NULL, 3,
        (1 << 1), NULL
    );

    xTaskCreateAffinitySet(
        taskTelemetry, "TelemetryTask",
        2048, NULL, 1,
        (1 << 0), NULL
    );

    vTaskStartScheduler();
}

void loop() {}