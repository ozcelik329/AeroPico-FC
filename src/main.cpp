#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "pico/time.h"
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

// Core 1 (uçuş kontrol döngüsü) taze mi? Bkz. SystemTimer::core1HeartbeatUs.
// Eşik: LOOP_TIME (2ms) döngüsüne göre bolca marj bırakan, ama donanım
// watchdog süresinden (WATCHDOG_TIMEOUT_MS) çok daha kısa bir değer.
static constexpr uint32_t CORE1_STALE_THRESHOLD_US = 20000; // 20 ms

static bool isCore1Alive() {
    uint32_t age = micros() - SystemTimer::getCore1HeartbeatUs();
    return age < CORE1_STALE_THRESHOLD_US;
}

void taskSensor(void* pvParameters) {
    for (;;) {
        flightManager.update();

        // Watchdog'u yalnızca Core 1 gerçekten taze/canlıysa besle.
        // Core 1 kilitlenirse besleme durur ve donanım watchdog'u
        // WATCHDOG_TIMEOUT_MS içinde chip'i resetler.
        if (isCore1Alive()) {
            watchdog_update();
        } else {
            Logger::log("[WATCHDOG] Core1 heartbeat bayat! Besleme durduruldu.");
        }

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