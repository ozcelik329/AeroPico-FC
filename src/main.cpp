#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "core/FlightManager.h"
#include "core/SystemTimer.h"
#include "core/WatchdogGate.h"
#include "core/Scheduler.h"
#include "core/PreflightHealth.h"
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
static constexpr uint32_t WATCHDOG_BLOCK_LOG_PERIOD_MS = 500;
static Scheduler telemetryScheduler;
static PreflightHealth preflightHealth;
static PreflightResult lastPreflightResult = {false, "not evaluated", 0};

static WatchdogDecision evaluateWatchdogGate() {
    return WatchdogGate::evaluate(
        micros(),
        SystemTimer::getCore1HeartbeatUs(),
        SystemTimer::is_running,
        SystemTimer::checkTimingBudgets(),
        CORE1_STALE_THRESHOLD_US
    );
}

static bool provideFlightData(FlightData& out) {
    return flightManager.peekLatest(out);
}

static bool provideArmState() {
    return flightManager.isArmed();
}

static void applyRCOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder) {
    flightManager.setRCOverride(aileron, elevator, throttle, rudder);
}

static void clearRCOverride() {
    flightManager.clearRCOverride();
}

static void applyPidGains(float angleP, float angleI, float angleD,
                          float rateP, float rateI, float rateD) {
    SystemTimer::applyPidGains(angleP, angleI, angleD, rateP, rateI, rateD);
}

static void applyMixerSettings(const MixerSettings& settings) {
    SystemTimer::applyMixerSettings(settings);
}

static void applyFailsafeTimeout(uint32_t timeoutMs) {
    rxManager.setFailsafeTimeoutMs(timeoutMs);
}

static PreflightResult evaluatePreflight() {
    preflightHealth.reset();
    preflightHealth.setCheck(PreflightCheckId::Boot, true, true, "");
    preflightHealth.setCheck(PreflightCheckId::Sensor, true, sensorManager.isImuAvailable(), "IMU not available");
    preflightHealth.setCheck(PreflightCheckId::RC, true, rxManager.isValid() && !rxManager.isFailsafe(), "RC signal invalid");
    preflightHealth.setCheck(PreflightCheckId::Failsafe, true, !rxManager.isFailsafe(), "RC failsafe active");
    preflightHealth.setCheck(PreflightCheckId::Scheduler, true, SystemTimer::checkTimingBudgets(), "Timing budget exceeded");
    preflightHealth.setCheck(PreflightCheckId::GPS, false, false, "GPS not configured");
    return preflightHealth.evaluate();
}

static void updatePreflightArmGate() {
    lastPreflightResult = evaluatePreflight();
    flightManager.setPreflightArmAllowed(lastPreflightResult.canArm);
}

static void runMavlinkTelemetry() {
    mavlink.update();
}

static void runBlackboxLog() {
    FlightData d;
    if (!flightManager.peekLatest(d)) {
        return;
    }

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
        d.failsafe,
        d.sensorHealth
    );
}

static void runHealthReport() {
    lastPreflightResult = evaluatePreflight();

    if (!lastPreflightResult.canArm) {
        mavlink.sendStatusText(lastPreflightResult.firstFailureReason);
    }

    if (!SystemTimer::checkTimingBudgets()) {
        TimingBudgetStatus status = SystemTimer::getTimingBudgetStatus();
        blackbox.logTimingBudget(status);
        mavlink.sendStatusText("Timing budget exceeded");
        SystemTimer::logTimingStats();
    }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    Serial.printf("[FREERTOS] Stack overflow in %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    while (true) {}
}

extern "C" void vApplicationMallocFailedHook() {
    Serial.println("[FREERTOS] Malloc failed!");
    taskDISABLE_INTERRUPTS();
    while (true) {}
}

void taskSensor(void* pvParameters) {
    uint32_t lastWatchdogBlockLogMs = 0;

    for (;;) {
        updatePreflightArmGate();
        flightManager.update();

        WatchdogDecision watchdogDecision = evaluateWatchdogGate();
        if (watchdogDecision.shouldFeed) {
            watchdog_update();
        } else {
            uint32_t nowMs = millis();
            if (nowMs - lastWatchdogBlockLogMs >= WATCHDOG_BLOCK_LOG_PERIOD_MS) {
                lastWatchdogBlockLogMs = nowMs;
                Serial.printf("[WATCHDOG] Besleme durduruldu: %s age=%uus\n",
                              watchdogDecision.reason,
                              watchdogDecision.heartbeatAgeUs);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void taskFlight(void* pvParameters) {
    SystemTimer::init();
    SystemTimer::core1_entry();
}

void taskTelemetry(void* pvParameters) {
    telemetryScheduler.reset();
    telemetryScheduler.addTask("mavlink", 20, runMavlinkTelemetry);
    telemetryScheduler.addTask("blackbox", 5, runBlackboxLog);
    telemetryScheduler.addTask("health", 1, runHealthReport);

    for (;;) {
        telemetryScheduler.tick(micros());
        vTaskDelay(pdMS_TO_TICKS(5));
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

    mavlink.setFlightDataProvider(provideFlightData);
    mavlink.setArmStateProvider(provideArmState);
    mavlink.setRCOverrideHandler(applyRCOverride);
    mavlink.setClearRCOverrideHandler(clearRCOverride);
    mavlink.init();
    blackbox.init();

#ifdef MAVLINK_PARAMS_ENABLED
    paramManager.setPidGainsApplyHandler(applyPidGains);
    paramManager.setMixerSettingsApplyHandler(applyMixerSettings);
    paramManager.setFailsafeTimeoutApplyHandler(applyFailsafeTimeout);
    paramManager.init();
    applyPidGains(
        paramManager.getAngleP(), paramManager.getAngleI(), paramManager.getAngleD(),
        paramManager.getRateP(), paramManager.getRateI(), paramManager.getRateD()
    );
    applyMixerSettings(paramManager.getMixerSettings());
    applyFailsafeTimeout(paramManager.getFailsafeTimeoutMs());
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
