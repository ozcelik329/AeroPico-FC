#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "board/Config.h"
#include "def.h"
#include "core/flight/FlightManager.h"
#include "core/scheduling/SystemTimer.h"
#include "core/safety/WatchdogGate.h"
#include "core/scheduling/Scheduler.h"
#include "core/safety/PreflightHealth.h"
#include "core/sensors/SensorPreflightEvaluator.h"
#include "core/safety/BatteryMonitor.h"
#include "core/events/SystemEventBus.h"
#include "storage/CalibrationStorage.h"
#include "storage/ParamStorage.h"
#include "utils/Logger.h"
#include "utils/BootLogger.h"
#include "app/MavlinkServiceCommands.h"
#include "app/ServiceCommandMailbox.h"
#include "app/ServiceCommandProcessor.h"
#include "telemetry/MavlinkHandler.h"
#include "telemetry/Blackbox.h"
#include "app/AppTasks.h"
#if BATTERY_ADC_ENABLED
#include "hal/rp2350/RP2350_ADC.h"
#endif
#ifdef MAVLINK_PARAMS_ENABLED
#include "telemetry/ParamManager.h"
#endif

FlightManager flightManager;

#include "drivers/Sensors.h"
#include "drivers/gps/GpsManager.h"
#include "drivers/RX.h"
SensorManager sensorManager;
RXManager rxManager;
static GpsManager gpsManager;

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
static constexpr uint32_t CONTROL_LOOP_HZ = 1000000UL / FLIGHT_LOOP_PERIOD_US;
static uint32_t lastBlackboxDroppedRecords = 0;
static bool batteryWarningLatched = false;
static bool latestBatteryCritical = false;
static bool magCalibrationActive = false;
static MavlinkServiceCommands mavlinkServiceCommands;
static ServiceCommandMailbox serviceCommandMailbox;
static ServiceCommandProcessor serviceCommandProcessor;
static TaskHandle_t sensorTaskHandle = nullptr;
static TaskHandle_t flightTaskHandle = nullptr;
static TaskHandle_t telemetryTaskHandle = nullptr;
static RuntimeHealthStatus runtimeHealth = {};

static PreflightResult evaluatePreflight();

static uint16_t clampStackWords(UBaseType_t value) { return value > 0xFFFFu ? 0xFFFFu : (uint16_t)value; }

static WatchdogDecision evaluateWatchdogGate() {
    return WatchdogGate::evaluate(
        micros(),
        SystemTimer::getCore1HeartbeatUs(),
        SystemTimer::is_running,
        SystemTimer::checkTimingBudgets(),
        CORE1_STALE_THRESHOLD_US
    );
}

static bool provideFlightData(FlightData& out) { return flightManager.peekLatest(out); }
static bool provideArmState() { return flightManager.isArmed(); }

static bool handleMavlinkArmCommand(bool arm, bool force, char* reason, size_t reasonLen) {
    return flightManager.requestArmFromMavlink(arm, force, reason, reasonLen);
}

static uint8_t handleMavlinkServiceCommand(uint16_t action, float p2, float p3, float p4, char* reason, size_t reasonLen) {
    return mavlinkServiceCommands.handle(action, p2, p3, p4, reason, reasonLen);
}

#if BATTERY_ADC_ENABLED
static bool provideBatteryVoltage(float& voltage) {
    return batteryAdc.readVoltage(BATTERY_ADC_CHANNEL, BATTERY_VOLTAGE_DIVIDER_RATIO, voltage);
}
#endif

static void applyRCOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder) { flightManager.setRCOverride(aileron, elevator, throttle, rudder); }
static void clearRCOverride() { flightManager.clearRCOverride(); }

static void applyPidGains(float angleP, float angleI, float angleD, float rateP, float rateI, float rateD) { SystemTimer::applyPidGains(angleP, angleI, angleD, rateP, rateI, rateD); }
static void applyMixerSettings(const MixerSettings& settings) { SystemTimer::applyMixerSettings(settings); }
static void applyFailsafeTimeout(uint32_t timeoutMs) { rxManager.setFailsafeTimeoutMs(timeoutMs); }
static void applyRcMapping(uint8_t roll, uint8_t pitch, uint8_t throttle, uint8_t yaw, uint8_t mode) {
    flightManager.applyRcMapping({roll, pitch, throttle, yaw, mode});
}
static void applyMavlinkRates(uint8_t attitudeHz, uint8_t rcHz, uint8_t sysStatusHz) { mavlink.setStreamRates(attitudeHz, rcHz, sysStatusHz); }
static void applyBlackboxRate(uint8_t logHz) { blackbox.setLogRateHz(logHz); }
static void applyPreflightQuality(uint8_t minQuality) { preflightMinSensorQuality = minQuality > 100 ? 100 : minQuality; }

static void applyBatteryProfile(uint8_t cells, float nominalVoltage, uint16_t capacityMah, uint8_t cRating,
                                float lowVoltage, float brownoutVoltage, float maxVoltage) {
    (void)nominalVoltage;
#if BATTERY_ADC_ENABLED
    batteryMonitor.init(provideBatteryVoltage, lowVoltage, maxVoltage, brownoutVoltage,
                        cells, capacityMah, cRating);
#else
    batteryMonitor.init(nullptr, lowVoltage, maxVoltage, brownoutVoltage,
                        cells, capacityMah, cRating);
#endif
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
    latestBatteryCritical = battery.configured && battery.brownout;

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

static void runSensorUpdate() { flightManager.updateSensors(); }
static void runServiceCommandMailbox() { serviceCommandProcessor.process(); }
static void runRcUpdate() { flightManager.updateRc(); }

static void runStatePublish() {
    flightManager.setSystemFaults(
        !SystemTimer::checkTimingBudgets(),
        latestBatteryCritical,
        !SystemTimer::outputsReady()
    );
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
    ServiceCommandCompletion completion = {};
    while (serviceCommandMailbox.takeCompletion(completion)) {
        if (completion.reason[0] != '\0') {
            mavlink.sendStatusText(
                completion.reason,
                completion.result == MAV_RESULT_ACCEPTED || completion.result == MAV_RESULT_IN_PROGRESS
                    ? MAV_SEVERITY_INFO
                    : MAV_SEVERITY_WARNING
            );
        }
    }
#ifdef MAVLINK_PARAMS_ENABLED
    paramManager.processSendQueue(millis());
#endif
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

static void runBlackboxDrain() { blackbox.drain(2); }

static void runHealthReport() {
    lastPreflightResult = evaluatePreflight();
    BatteryStatus battery = batteryMonitor.evaluate();
    latestBatteryCritical = battery.configured && battery.brownout;
    runtimeHealth.sensorStackHighWaterWords = sensorTaskHandle
        ? clampStackWords(uxTaskGetStackHighWaterMark(sensorTaskHandle)) : 0;
    runtimeHealth.flightStackHighWaterWords = flightTaskHandle
        ? clampStackWords(uxTaskGetStackHighWaterMark(flightTaskHandle)) : 0;
    runtimeHealth.telemetryStackHighWaterWords = telemetryTaskHandle
        ? clampStackWords(uxTaskGetStackHighWaterMark(telemetryTaskHandle)) : 0;
    runtimeHealth.eventQueueDrops = systemEventBus.droppedCount() > 0xFFFFu
        ? 0xFFFFu : (uint16_t)systemEventBus.droppedCount();

    if (!lastPreflightResult.canArm) {
        mavlink.sendStatusText(lastPreflightResult.firstFailureReason);
    }

    if (battery.configured && !battery.healthy && !batteryWarningLatched) {
        batteryWarningLatched = true;
        systemEventBus.publish({
            SystemEventType::BatteryWarning,
            micros(),
            battery.brownout ? 2u : 1u
        });
        mavlink.sendStatusText(battery.reason);
    } else if (battery.configured && battery.healthy) {
        batteryWarningLatched = false;
    }

    if (sensorManager.getFaultCode() != SensorFaultCode::None) {
        mavlink.sendStatusText(sensorManager.getFaultText());
    }

    if (!SystemTimer::checkTimingBudgets()) {
        TimingBudgetStatus status = SystemTimer::getTimingBudgetStatus();
        blackbox.logTimingBudget(status);
        mavlink.sendStatusText("Timing budget exceeded");
        systemEventBus.publish({
            SystemEventType::TimingOverrun,
            micros(),
            ((uint32_t)status.totalDeadlineMisses << 16) | status.totalLoadPermille
        });
    }
    const uint32_t droppedBlackbox = blackbox.droppedRecords();
    runtimeHealth.blackboxDrops = droppedBlackbox > 0xFFFFu ? 0xFFFFu : (uint16_t)droppedBlackbox;
    blackbox.logRuntimeHealth(runtimeHealth);
    if (droppedBlackbox != lastBlackboxDroppedRecords) {
        lastBlackboxDroppedRecords = droppedBlackbox;
        systemEventBus.publish({
            SystemEventType::BlackboxDrop,
            micros(),
            droppedBlackbox
        });
        mavlink.sendStatusText("Blackbox records dropped");
    }
    SystemTimer::requestTimingWindowReset();
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
    core0Scheduler.addTask("service", 50, runServiceCommandMailbox);
    core0Scheduler.addTask("rc", 150, runRcUpdate);
    core0Scheduler.addTask("state", 200, runStatePublish);
    core0Scheduler.addTask("preflight", 20, updatePreflightArmGate);
    core0Scheduler.addTask("watchdog", 100, runWatchdogGate);

    for (;;) {
        core0Scheduler.tick(micros());
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void taskFlight(void* pvParameters) { SystemTimer::init(); SystemTimer::core1_entry(); }

void taskTelemetry(void* pvParameters) {
    telemetryScheduler.reset();
    telemetryScheduler.addTask("mavlink", 50, runMavlinkTelemetry);
    telemetryScheduler.addTask("blackbox-drain", 100, runBlackboxDrain);
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
    batteryMonitor.init(provideBatteryVoltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE, BATTERY_BROWNOUT_VOLTAGE);
#else
    batteryMonitor.init();
#endif
    flightManager.init(&sensorManager, &rxManager);

    SensorCapabilityStatus sensorCapabilities = sensorManager.capabilities();
    bool imuOk = sensorCapabilities.imuAvailable;
    if (imuOk) {
        char whoamiText[16];
        snprintf(whoamiText, sizeof(whoamiText), "WHOAMI=0x%02X", sensorManager.getLastWhoAmI());
        BootLogger::okWithValue("MPU6050", whoamiText);

        CalibrationBlob calibrationBlob = {};
        if (calibrationStorage.load(calibrationBlob)) {
            sensorManager.setImuCalibration(calibrationBlob.imu);
            sensorManager.setMagCalibration(calibrationBlob.mag);
            BootLogger::ok("Calibration Load");
        } else if (sensorManager.runBootCalibration()) {
            BootLogger::ok("Gyro/Accel Bias Cal");
            CalibrationBlob savedCalibration = CalibrationStorage::makeBlob(sensorManager.getImuCalibration(),
                                                                            sensorManager.getMagCalibration());
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
    sensorCapabilities = sensorManager.capabilities();
    if (sensorCapabilities.baroAvailable) BootLogger::ok("BMP085");
    else BootLogger::fail("BMP085", "Barometre bulunamadi");

    if (sensorCapabilities.magAvailable) BootLogger::ok("HMC5883L");
    else BootLogger::fail("HMC5883L", "Manyetometre bulunamadi");
#endif

    gpsManager.init(nullptr, GPS_MODULE_ENABLED, GPS_UART_BAUD);
    if (GPS_MODULE_ENABLED) BootLogger::warn("GPS", "UART baglantisi bekleniyor");
    else BootLogger::warn("GPS", "Kapali; takili degilse navigation devreye girmez");

    BootLogger::ok("RC Receiver (SBUS/UART0)");

    MavlinkServiceContext serviceContext = {};
    serviceContext.sensors = &sensorManager;
    serviceContext.receiver = &rxManager;
    serviceContext.calibrationStorage = &calibrationStorage;
    serviceContext.magCalibrationActive = &magCalibrationActive;
    serviceContext.isArmed = provideArmState;
    serviceContext.evaluatePreflight = evaluatePreflight;
    serviceContext.requestServoTest = SystemTimer::requestServoTest;
    serviceContext.lastPreflightResult = &lastPreflightResult;
    serviceContext.mailbox = &serviceCommandMailbox;
    mavlinkServiceCommands.init(serviceContext);

    ServiceCommandProcessorContext serviceProcessorContext = {};
    serviceProcessorContext.sensors = &sensorManager;
    serviceProcessorContext.calibrationStorage = &calibrationStorage;
    serviceProcessorContext.magCalibrationActive = &magCalibrationActive;
    serviceProcessorContext.isArmed = provideArmState;
    serviceProcessorContext.requestServoTest = SystemTimer::requestServoTest;
    serviceProcessorContext.mailbox = &serviceCommandMailbox;
    serviceCommandProcessor.init(serviceProcessorContext);

    mavlink.setFlightDataProvider(provideFlightData);
    mavlink.setArmStateProvider(provideArmState);
    mavlink.setArmCommandHandler(handleMavlinkArmCommand);
    mavlink.setServiceCommandHandler(handleMavlinkServiceCommand);
    mavlink.setRCOverrideHandler(applyRCOverride);
    mavlink.setClearRCOverrideHandler(clearRCOverride);
    mavlink.setRCOverrideEnabled(true);
    mavlink.setRCOverrideAllowedWhileArmed(false);
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
    paramManager.setBatteryProfileApplyHandler(applyBatteryProfile);
    paramManager.setArmStateProvider(provideArmState);
    paramManager.setStorage(&paramStorage);
    paramManager.init();
    applyPidGains(paramManager.getAngleP(), paramManager.getAngleI(), paramManager.getAngleD(),
                  paramManager.getRateP(), paramManager.getRateI(), paramManager.getRateD());
    applyMixerSettings(paramManager.getMixerSettings());
    applyFailsafeTimeout(paramManager.getFailsafeTimeoutMs());
    applyRcMapping(paramManager.getRcRollChannel(), paramManager.getRcPitchChannel(),
                   paramManager.getRcThrottleChannel(), paramManager.getRcYawChannel(),
                   paramManager.getRcModeChannel());
    applyMavlinkRates(paramManager.getMavlinkAttitudeHz(),
                      paramManager.getMavlinkRcHz(),
                      paramManager.getMavlinkSysStatusHz());
    applyBlackboxRate(paramManager.getBlackboxLogHz());
    applyPreflightQuality(paramManager.getPreflightMinQuality());
    applyBatteryProfile(paramManager.getBatteryCellCount(), paramManager.getBatteryNominalVoltage(),
                        paramManager.getBatteryCapacityMah(), paramManager.getBatteryCRating(),
                        paramManager.getBatteryLowVoltage(), paramManager.getBatteryBrownoutVoltage(),
                        paramManager.getBatteryMaxVoltage());
#endif

    sensorCapabilities = sensorManager.capabilities();
    SensorCapabilityStatus gpsCapabilities = gpsManager.capabilities();
    const uint16_t functionMask = sensorCapabilities.functionMask | gpsCapabilities.functionMask;
    bool baroOk = hasSensorCapability(functionMask, SENSOR_CAP_BARO);
    bool magOk  = hasSensorCapability(functionMask, SENSOR_CAP_MAG);
    bool dmaOk  = sensorManager.isDmaOk();
    bool rxOk   = true;

    BootLogger::printHealthReport(
        CONTROL_LOOP_HZ,
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

    const AppTaskHandles taskHandles = AppTasks::create(taskSensor, taskFlight, taskTelemetry);
    sensorTaskHandle = taskHandles.sensor; flightTaskHandle = taskHandles.flight; telemetryTaskHandle = taskHandles.telemetry;

    vTaskStartScheduler();
}

void loop() {}
