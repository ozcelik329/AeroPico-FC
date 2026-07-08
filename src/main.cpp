#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "config.h"
#include "core/FlightManager.h"
#include "core/SystemTimer.h"
#include "core/WatchdogGate.h"
#include "core/Scheduler.h"
#include "core/PreflightHealth.h"
#include "core/SensorPreflightEvaluator.h"
#include "core/BatteryMonitor.h"
#include "storage/CalibrationStorage.h"
#include "storage/ParamStorage.h"
#include "utils/Logger.h"
#include "utils/BootLogger.h"
#include "telemetry/MavlinkHandler.h"
#include "telemetry/Blackbox.h"
#if BATTERY_ADC_ENABLED
#include "hal/rp2350/RP2350_ADC.h"
#endif
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
static Scheduler core0Scheduler;
static Scheduler telemetryScheduler;
static PreflightHealth preflightHealth;
static BatteryMonitor batteryMonitor;
static RPFlashCalibrationStorage calibrationStorage;
#ifdef MAVLINK_PARAMS_ENABLED
static RPFlashParamStorage paramStorage;
#endif
#if BATTERY_ADC_ENABLED
static RP2350ADC batteryAdc;
#endif
static PreflightResult lastPreflightResult = {false, "not evaluated", 0};
static constexpr uint32_t PREFLIGHT_MIN_FREE_HEAP_BYTES = 24 * 1024;
static uint8_t preflightMinSensorQuality = 60;
static char sensorPreflightReason[72] = "Sensor not evaluated";
static uint32_t lastWatchdogBlockLogMs = 0;

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

#if BATTERY_ADC_ENABLED
static bool provideBatteryVoltage(float& voltage) {
    return batteryAdc.readVoltage(BATTERY_ADC_CHANNEL, BATTERY_VOLTAGE_DIVIDER_RATIO, voltage);
}
#endif

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

static void applyRcMapping(uint8_t roll, uint8_t pitch, uint8_t throttle, uint8_t yaw) {
    flightManager.applyRcMapping({roll, pitch, throttle, yaw});
}

static void applyMavlinkRates(uint8_t attitudeHz, uint8_t rcHz, uint8_t sysStatusHz) {
    mavlink.setStreamRates(attitudeHz, rcHz, sysStatusHz);
}

static void applyBlackboxRate(uint8_t logHz) {
    blackbox.setLogRateHz(logHz);
}

static void applyPreflightQuality(uint8_t minQuality) {
    preflightMinSensorQuality = minQuality > 100 ? 100 : minQuality;
}

static bool evaluateSensorPreflight() {
    SensorBuffer latest = sensorManager.getLatest();
    const SensorPreflightStatus status = SensorPreflightEvaluator::evaluate(
        sensorManager.isImuAvailable(),
        latest,
        preflightMinSensorQuality
    );
    SensorPreflightEvaluator::formatReason(status, sensorPreflightReason, sizeof(sensorPreflightReason));
    return status.passed;
}

static PreflightResult evaluatePreflight() {
    BatteryStatus battery = batteryMonitor.evaluate();
    uint32_t freeHeap = rp2040.getFreeHeap();
    bool sensorOk = evaluateSensorPreflight();

    preflightHealth.reset();
    preflightHealth.setCheck(PreflightCheckId::Boot, true, true, "");
    preflightHealth.setCheck(PreflightCheckId::Sensor, true, sensorOk, sensorPreflightReason);
    preflightHealth.setCheck(PreflightCheckId::RC, true, rxManager.isValid() && !rxManager.isFailsafe(), "RC signal invalid");
    preflightHealth.setCheck(PreflightCheckId::Battery, battery.configured, battery.healthy, battery.reason);
    preflightHealth.setCheck(PreflightCheckId::Memory, true, freeHeap >= PREFLIGHT_MIN_FREE_HEAP_BYTES, "Free heap too low");
    preflightHealth.setCheck(PreflightCheckId::Actuator, true, SystemTimer::outputsReady(), "Actuator output not ready");
    preflightHealth.setCheck(PreflightCheckId::Failsafe, true, !rxManager.isFailsafe(), "RC failsafe active");
    preflightHealth.setCheck(PreflightCheckId::Scheduler, true, SystemTimer::checkTimingBudgets(), "Timing budget exceeded");
    preflightHealth.setCheck(PreflightCheckId::GPS, false, false, "GPS not configured");
    return preflightHealth.evaluate();
}

static void updatePreflightArmGate() {
    lastPreflightResult = evaluatePreflight();
    flightManager.setPreflightArmAllowed(lastPreflightResult.canArm);
}

static void runSensorUpdate() {
    flightManager.updateSensors();
}

static void runRcUpdate() {
    flightManager.updateRc();
}

static void runStatePublish() {
    flightManager.publishState();
}

static void runWatchdogGate() {
    WatchdogDecision watchdogDecision = evaluateWatchdogGate();
    if (watchdogDecision.shouldFeed) {
        watchdog_update();
        return;
    }

    uint32_t nowMs = millis();
    if (nowMs - lastWatchdogBlockLogMs >= WATCHDOG_BLOCK_LOG_PERIOD_MS) {
        lastWatchdogBlockLogMs = nowMs;
        Serial.printf("[WATCHDOG] Besleme durduruldu: %s age=%uus\n",
                      watchdogDecision.reason,
                      watchdogDecision.heartbeatAgeUs);
    }
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

    if (sensorManager.getFaultCode() != SensorFaultCode::None) {
        mavlink.sendStatusText(sensorManager.getFaultText());
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
    core0Scheduler.reset();
    core0Scheduler.addTask("sensor", 200, runSensorUpdate);
    core0Scheduler.addTask("rc", 50, runRcUpdate);
    core0Scheduler.addTask("state", 200, runStatePublish);
    core0Scheduler.addTask("preflight", 20, updatePreflightArmGate);
    core0Scheduler.addTask("watchdog", 100, runWatchdogGate);

    for (;;) {
        core0Scheduler.tick(micros());
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void taskFlight(void* pvParameters) {
    SystemTimer::init();
    SystemTimer::core1_entry();
}

void taskTelemetry(void* pvParameters) {
    telemetryScheduler.reset();
    telemetryScheduler.addTask("mavlink", 50, runMavlinkTelemetry);
    telemetryScheduler.addTask("blackbox", 50, runBlackboxLog);
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
#if BATTERY_ADC_ENABLED
    batteryAdc.init(PIN_BATTERY_ADC, BATTERY_ADC_CHANNEL);
    batteryMonitor.init(provideBatteryVoltage,
                        BATTERY_MIN_VOLTAGE,
                        BATTERY_MAX_VOLTAGE,
                        BATTERY_BROWNOUT_VOLTAGE);
#else
    batteryMonitor.init();
#endif
    flightManager.init(&sensorManager, &rxManager);

    bool imuOk = sensorManager.isImuAvailable();
    if (imuOk) {
        BootLogger::okWithValue("MPU6050", "WHOAMI=0x68");

        CalibrationBlob calibrationBlob = {};
        if (calibrationStorage.load(calibrationBlob)) {
            sensorManager.setImuCalibration(calibrationBlob.imu);
            sensorManager.setMagCalibration(calibrationBlob.mag);
            BootLogger::ok("Calibration Load");
        } else if (sensorManager.runBootCalibration()) {
            BootLogger::ok("Gyro/Accel Bias Cal");
            CalibrationBlob savedCalibration = CalibrationStorage::makeBlob(
                sensorManager.getImuCalibration(),
                sensorManager.getMagCalibration()
            );
            if (calibrationStorage.save(savedCalibration)) {
                BootLogger::ok("Calibration Save");
            } else {
                BootLogger::warn("Calibration Save", "Flash kaydi basarisiz");
            }
        } else {
            BootLogger::warn("Gyro/Accel Bias Cal", "Yetersiz ornek veya basarisiz");
        }
    } else {
        BootLogger::fail("MPU6050", "WHOAMI dogrulanamadi veya bagli degil");
        BootLogger::warn("Sensor Fault", sensorManager.getFaultText());
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
    paramManager.setRcMappingApplyHandler(applyRcMapping);
    paramManager.setMavlinkRatesApplyHandler(applyMavlinkRates);
    paramManager.setBlackboxRateApplyHandler(applyBlackboxRate);
    paramManager.setPreflightQualityApplyHandler(applyPreflightQuality);
    paramManager.setStorage(&paramStorage);
    paramManager.init();
    applyPidGains(
        paramManager.getAngleP(), paramManager.getAngleI(), paramManager.getAngleD(),
        paramManager.getRateP(), paramManager.getRateI(), paramManager.getRateD()
    );
    applyMixerSettings(paramManager.getMixerSettings());
    applyFailsafeTimeout(paramManager.getFailsafeTimeoutMs());
    applyRcMapping(
        paramManager.getRcRollChannel(),
        paramManager.getRcPitchChannel(),
        paramManager.getRcThrottleChannel(),
        paramManager.getRcYawChannel()
    );
    applyMavlinkRates(
        paramManager.getMavlinkAttitudeHz(),
        paramManager.getMavlinkRcHz(),
        paramManager.getMavlinkSysStatusHz()
    );
    applyBlackboxRate(paramManager.getBlackboxLogHz());
    applyPreflightQuality(paramManager.getPreflightMinQuality());
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
