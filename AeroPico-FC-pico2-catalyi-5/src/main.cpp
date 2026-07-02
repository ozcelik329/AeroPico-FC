#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "core/FlightManager.h"
#include "core/SystemTimer.h"
#include "utils/Logger.h"
#include "utils/BootLogger.h"
#include "telemetry/MavlinkHandler.h"
#include "telemetry/Blackbox.h"
#ifdef MAVLINK_PARAMS_ENABLED
#include "telemetry/ParamManager.h"
#endif

FlightManager flightManager;

#include "drivers/Sensors.h"
#include "drivers/RX.h"
SensorManager sensorManager;
RXManager rxManager;

static constexpr uint32_t CORE1_STALE_THRESHOLD_US = 20000;

static bool isCore1Alive() {
    uint32_t age = micros() - SystemTimer::getCore1HeartbeatUs();
    return age < CORE1_STALE_THRESHOLD_US;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    Serial.printf("[FREERTOS] Stack overflow in %s\n", pcTaskName);
    watchdog_update();
    taskDISABLE_INTERRUPTS();
    while (true) {}
}

extern "C" void vApplicationMallocFailedHook() {
    Serial.println("[FREERTOS] Malloc failed!");
    watchdog_update();
    taskDISABLE_INTERRUPTS();
    while (true) {}
}

void taskSensor(void* pvParameters) {
    for (;;) {
        flightManager.update();

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
        mavlink.update();

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
                d.failsafe
            );
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    BootLogger::printBanner();

    if (watchdog_caused_reboot()) {
        BootLogger::warn("Watchdog", "Onceki oturum watchdog ile resetlendi");
    }

    Logger::init();
    flightManager.init(&sensorManager, &rxManager);

    bool imuOk = sensorManager.isImuAvailable();
    if (imuOk) {
        BootLogger::okWithValue("MPU6050", "WHOAMI=0x68");
        if (sensorManager.runBootCalibration()) {
            BootLogger::ok("Gyro/Accel Bias Cal");
        } else {
            BootLogger::warn("Gyro/Accel Bias Cal", "Yetersiz ornek veya basarisiz");
        }
    } else {
        BootLogger::fail("MPU6050", "WHOAMI dogrulanamadi veya bagli degil");
    }

#ifdef USE_GY87
    if (sensorManager.hasBaro()) BootLogger::ok("BMP085");
    else BootLogger::fail("BMP085", "Barometre bulunamadi");

    if (sensorManager.hasMag()) BootLogger::ok("HMC5883L");
    else BootLogger::fail("HMC5883L", "Manyetometre bulunamadi");
#endif

    BootLogger::ok("RC Receiver (SBUS/UART0)");

    mavlink.init();
    blackbox.init();

#ifdef MAVLINK_PARAMS_ENABLED
    paramManager.init();
    SystemTimer::applyPidGains(
        paramManager.getAngleP(), paramManager.getAngleI(), paramManager.getAngleD(),
        paramManager.getRateP(), paramManager.getRateI(), paramManager.getRateD()
    );
#endif

    bool baroOk = sensorManager.hasBaro();
    bool magOk  = sensorManager.hasMag();
    bool dmaOk  = sensorManager.isDmaOk();
    bool rxOk   = true;

    BootLogger::printHealthReport(
        500,
        imuOk,
        baroOk,
        magOk,
        rxOk,
        dmaOk,
        false,
        false,
        rp2040.getFreeHeap()
    );
    BootLogger::printReadyMessage();

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
