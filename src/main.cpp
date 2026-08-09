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
#include "core/safety/BenchAdminGate.h"
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
#if BLACKBOX_SD_ENABLED
#include "hal/rp2350/RP2350_SPI.h"
#include "telemetry/BlackboxSdSink.h"
#endif
#include "app/AppTasks.h"
#if BATTERY_ADC_ENABLED
#include "hal/rp2350/RP2350_ADC.h"
#endif
#ifdef MAVLINK_PARAMS_ENABLED
#include "telemetry/ParamManager.h"
#endif
#include "drivers/Sensors.h"
#include "drivers/Output.h"
#include "drivers/gps/GpsManager.h"
#include "drivers/RX.h"
FlightManager flightManager;
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
#if BLACKBOX_SD_ENABLED
static RP2350SPI blackboxSpi;
static BlackboxSdSink blackboxSdSink(PIN_BLACKBOX_SPI_CS, BLACKBOX_SPI_HZ, BLACKBOX_SD_FILE);
#endif
static PreflightResult lastPreflightResult = {false, "not evaluated", 0};
static constexpr uint32_t PREFLIGHT_MIN_FREE_HEAP_BYTES = 24 * 1024;
static uint8_t preflightMinSensorQuality = 60;
static char sensorPreflightReason[72] = "Sensor not evaluated";
static char moduleSetupPreflightReason[96] = "Module setup not evaluated";
static uint32_t lastWatchdogBlockLogMs = 0;
static constexpr uint32_t CONTROL_LOOP_HZ = 1000000UL / FLIGHT_LOOP_PERIOD_US;
static uint32_t lastBlackboxDroppedRecords = 0;
static bool batteryWarningLatched = false;
static bool latestBatteryCritical = false;
static bool magCalibrationActive = false;
static uint8_t batteryAdcChannel = BATTERY_ADC_CHANNEL;
static bool baroModuleEnabled = true;
static bool magModuleEnabled = true;
static bool gpsModuleEnabled = GPS_MODULE_ENABLED != 0;
static bool batteryModuleEnabled = false;
static bool rcModuleEnabled = true;
static uint8_t imuModuleType = 1;
static uint8_t baroModuleType = 1;
static uint8_t magModuleType = 0;
static uint8_t gpsModuleType = 1;
static uint8_t rcModuleType = 1;
static uint8_t batteryModuleType = 0;
static bool bootServoPinConfigValid = true;
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
    const bool accepted = flightManager.requestArmFromMavlink(arm, force, reason, reasonLen);
    if (!accepted && reason && strcmp(reason, "preflight blocked") == 0 &&
        lastPreflightResult.firstFailureReason && lastPreflightResult.firstFailureReason[0] != '\0') {
        strncpy(reason, lastPreflightResult.firstFailureReason, reasonLen - 1);
        reason[reasonLen - 1] = '\0';
    }
    return accepted;
}

static uint8_t handleMavlinkServiceCommand(uint16_t action, float p2, float p3, float p4, char* reason, size_t reasonLen) {
    return mavlinkServiceCommands.handle(action, p2, p3, p4, reason, reasonLen);
}

#if BATTERY_ADC_ENABLED
static bool provideBatteryVoltage(float& voltage) {
    return batteryAdc.readVoltage(batteryAdcChannel, BATTERY_VOLTAGE_DIVIDER_RATIO, voltage);
}
#endif

static uint8_t adcChannelForPin(uint8_t pin) {
    if (pin < 26 || pin > 28) {
        return BATTERY_ADC_CHANNEL;
    }
    return (uint8_t)(pin - 26);
}

static void refreshModuleSetupFromParams() {
#ifdef MAVLINK_PARAMS_ENABLED
    baroModuleEnabled = paramManager.isBaroEnabled();
    magModuleEnabled = paramManager.isMagEnabled();
    gpsModuleEnabled = paramManager.isGpsEnabled();
    batteryModuleEnabled = paramManager.isBatteryEnabled();
    rcModuleEnabled = paramManager.isRcEnabled();
    imuModuleType = paramManager.getImuType();
    baroModuleType = paramManager.getBaroType();
    magModuleType = paramManager.getMagType();
    gpsModuleType = paramManager.getGpsType();
    rcModuleType = paramManager.getRcType();
    batteryModuleType = paramManager.getBatteryType();
    flightManager.setRcRequired(rcModuleEnabled && rcModuleType != 0);
#endif
}

static bool isSupportedImuType(uint8_t type) { return type <= 1; }      // 0=Auto, 1=MPU6050
static bool isSupportedBaroType(uint8_t type) { return type <= 1; }     // 0=Auto, 1=BMP180/BMP085
static bool isSupportedMagType(uint8_t type) { return type <= 2; }      // 0=Auto, 1=HMC5883L, 2=QMC5883
static bool isSupportedGpsType(uint8_t type) { return type <= 1; }      // 0=Auto, 1=NMEA UART
static bool isSupportedRcType(uint8_t type) { return type <= 1; }       // 0=Auto, 1=SBUS
static bool isSupportedBatteryType(uint8_t type) { return type <= 1; }  // 0=None, 1=ADC voltage

static ServoPinConfig servoPinsFromParams() {
#ifdef MAVLINK_PARAMS_ENABLED
    return {
        paramManager.getPinAileron(),
        paramManager.getPinElevator(),
        paramManager.getPinRudder(),
        paramManager.getPinThrottle()
    };
#else
    return {PIN_AILERON, PIN_ELEVATOR, PIN_RUDDER, PIN_THROTTLE};
#endif
}

static bool isReservedServoSetupPin(uint8_t pin) {
    return pin == PIN_SBUS_RX ||
           pin == PIN_SDA ||
           pin == PIN_SCL ||
           pin == PIN_BENCH_ADMIN_GND ||
           pin == PIN_BENCH_ADMIN_SENSE;
}

static bool isValidServoSetupPin(uint8_t pin) {
    return pin <= 28 && !isReservedServoSetupPin(pin);
}

static bool validateServoPinSetup(const ServoPinConfig& pins) {
    if (!isValidServoSetupPin(pins.aileron) ||
        !isValidServoSetupPin(pins.elevator) ||
        !isValidServoSetupPin(pins.rudder) ||
        !isValidServoSetupPin(pins.throttle)) {
        return false;
    }

    return pins.aileron != pins.elevator &&
           pins.aileron != pins.rudder &&
           pins.aileron != pins.throttle &&
           pins.elevator != pins.rudder &&
           pins.elevator != pins.throttle &&
           pins.rudder != pins.throttle;
}

static uint16_t enabledSensorMask(uint16_t detectedMask) {
    refreshModuleSetupFromParams();
    uint16_t mask = detectedMask;
    if (!baroModuleEnabled) mask &= ~(uint16_t)SENSOR_CAP_BARO;
    if (!magModuleEnabled) mask &= ~(uint16_t)SENSOR_CAP_MAG;
    if (!gpsModuleEnabled) mask &= ~(uint16_t)SENSOR_CAP_GPS;
    return mask;
}

static uint16_t provideEnabledSensorCapabilities() {
    const SensorCapabilityStatus sensorCaps = sensorManager.capabilities();
    const SensorCapabilityStatus gpsCaps = gpsManager.capabilities();
    return enabledSensorMask(sensorCaps.functionMask | gpsCaps.functionMask);
}

static void applyRCOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder) { flightManager.setRCOverride(aileron, elevator, throttle, rudder); }
static void clearRCOverride() { flightManager.clearRCOverride(); }
static void applyPidGains(float angleP, float angleI, float angleD, float rateP, float rateI, float rateD) { SystemTimer::applyPidGains(angleP, angleI, angleD, rateP, rateI, rateD); }
static void applyMixerSettings(const MixerSettings& settings) { SystemTimer::applyMixerSettings(settings); }
static void applyFailsafeTimeout(uint32_t timeoutMs) { rxManager.setFailsafeTimeoutMs(timeoutMs); }
static void applyRcMapping(uint8_t roll, uint8_t pitch, uint8_t throttle, uint8_t yaw, uint8_t mode) {
    flightManager.applyRcMapping({roll, pitch, throttle, yaw, mode});
}
static void provideRcMapping(uint8_t& roll, uint8_t& pitch, uint8_t& throttle, uint8_t& yaw, uint8_t& mode) {
#ifdef MAVLINK_PARAMS_ENABLED
    roll = paramManager.getRcRollChannel();
    pitch = paramManager.getRcPitchChannel();
    throttle = paramManager.getRcThrottleChannel();
    yaw = paramManager.getRcYawChannel();
    mode = paramManager.getRcModeChannel();
#else
    roll = RC_ROLL_CHANNEL;
    pitch = RC_PITCH_CHANNEL;
    throttle = RC_THROTTLE_CHANNEL;
    yaw = RC_YAW_CHANNEL;
    mode = RC_MODE_CHANNEL;
#endif
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

static bool evaluateModuleSetupPreflight() {
    refreshModuleSetupFromParams();

    const ServoPinConfig servoPins = servoPinsFromParams();
    if (!bootServoPinConfigValid || !validateServoPinSetup(servoPins)) {
        strncpy(moduleSetupPreflightReason, "Setup servo pin map invalid", sizeof(moduleSetupPreflightReason) - 1);
        moduleSetupPreflightReason[sizeof(moduleSetupPreflightReason) - 1] = '\0';
        return false;
    }

    const SensorCapabilityStatus sensorCaps = sensorManager.capabilities();
    const SensorCapabilityStatus gpsCaps = gpsManager.capabilities();
    const uint16_t detectedMask = sensorCaps.functionMask | gpsCaps.functionMask;

    if (!isSupportedImuType(imuModuleType) ||
        !isSupportedBaroType(baroModuleType) ||
        !isSupportedMagType(magModuleType) ||
        !isSupportedGpsType(gpsModuleType) ||
        !isSupportedRcType(rcModuleType) ||
        !isSupportedBatteryType(batteryModuleType)) {
        strncpy(moduleSetupPreflightReason, "Setup contains unsupported module type", sizeof(moduleSetupPreflightReason) - 1);
        moduleSetupPreflightReason[sizeof(moduleSetupPreflightReason) - 1] = '\0';
        return false;
    }

    if (batteryModuleEnabled && batteryModuleType == 0) {
        strncpy(moduleSetupPreflightReason, "Setup enables battery but type is None", sizeof(moduleSetupPreflightReason) - 1);
        moduleSetupPreflightReason[sizeof(moduleSetupPreflightReason) - 1] = '\0';
        return false;
    }

    if (baroModuleEnabled && !hasSensorCapability(detectedMask, SENSOR_CAP_BARO)) {
        strncpy(moduleSetupPreflightReason, "Setup requires BARO but barometer is missing", sizeof(moduleSetupPreflightReason) - 1);
        moduleSetupPreflightReason[sizeof(moduleSetupPreflightReason) - 1] = '\0';
        return false;
    }
    if (magModuleEnabled && !hasSensorCapability(detectedMask, SENSOR_CAP_MAG)) {
        strncpy(moduleSetupPreflightReason, "Setup requires MAG but magnetometer is missing", sizeof(moduleSetupPreflightReason) - 1);
        moduleSetupPreflightReason[sizeof(moduleSetupPreflightReason) - 1] = '\0';
        return false;
    }
    if (gpsModuleEnabled && !hasSensorCapability(detectedMask, SENSOR_CAP_GPS)) {
        strncpy(moduleSetupPreflightReason, "Setup requires GPS but no valid fix is available", sizeof(moduleSetupPreflightReason) - 1);
        moduleSetupPreflightReason[sizeof(moduleSetupPreflightReason) - 1] = '\0';
        return false;
    }

    snprintf(moduleSetupPreflightReason,
             sizeof(moduleSetupPreflightReason),
             "Module setup OK BARO=%u MAG=%u GPS=%u BATT=%u",
             baroModuleEnabled ? 1u : 0u,
             magModuleEnabled ? 1u : 0u,
             gpsModuleEnabled ? 1u : 0u,
             batteryModuleEnabled ? 1u : 0u);
    return true;
}

static PreflightResult evaluatePreflight() {
    BatteryStatus battery = batteryMonitor.evaluate();
    uint32_t freeHeap = rp2040.getFreeHeap();
    bool sensorOk = evaluateSensorPreflight();
    bool moduleSetupOk = evaluateModuleSetupPreflight();
    const bool batteryRequired = batteryModuleEnabled;
    const bool rcRequired = rcModuleEnabled && rcModuleType != 0;
    const bool batteryOk = !batteryRequired || (battery.configured && battery.healthy);
    const bool rcOk = rxManager.isValid() && !rxManager.isFailsafe();
    latestBatteryCritical = batteryRequired && battery.configured && battery.brownout;

    preflightHealth.reset();
    preflightHealth.setCheck(PreflightCheckId::Boot, true, true, "");
    preflightHealth.setCheck(PreflightCheckId::Sensor, true, sensorOk, sensorPreflightReason);
    preflightHealth.setCheck(PreflightCheckId::RC,
                             rcRequired,
                             rcOk,
                             rcRequired ? "RC signal invalid" : "RC disabled in setup");
    preflightHealth.setCheck(PreflightCheckId::ModuleSetup, true, moduleSetupOk, moduleSetupPreflightReason);
    preflightHealth.setCheck(PreflightCheckId::Battery,
                             batteryRequired,
                             batteryOk,
                             batteryRequired ? battery.reason : "Battery disabled in setup");
    preflightHealth.setCheck(PreflightCheckId::Memory, true, freeHeap >= PREFLIGHT_MIN_FREE_HEAP_BYTES, "Free heap too low");
    preflightHealth.setCheck(PreflightCheckId::Actuator, true, SystemTimer::outputsReady(), "Actuator output not ready");
    preflightHealth.setCheck(PreflightCheckId::Failsafe,
                             rcRequired,
                             !rxManager.isFailsafe(),
                             rcRequired ? "RC failsafe active" : "RC disabled in setup");
    preflightHealth.setCheck(PreflightCheckId::Scheduler, true, SystemTimer::checkTimingBudgets(), "Timing budget exceeded");
    preflightHealth.setCheck(PreflightCheckId::GPS, false, false, "GPS not configured");
    return preflightHealth.evaluate();
}

static void updatePreflightArmGate() {
    lastPreflightResult = evaluatePreflight();
    flightManager.setBenchForceArmAllowed(benchAdminGate.forceArmActive());
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
    refreshModuleSetupFromParams();
    latestBatteryCritical = batteryModuleEnabled && battery.configured && battery.brownout;
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
    benchAdminGate.init();
#ifdef MAVLINK_PARAMS_ENABLED
    paramManager.setArmStateProvider(provideArmState);
    paramManager.setStorage(&paramStorage);
    paramManager.init();
    const ServoPinConfig servoPins = servoPinsFromParams();
    bootServoPinConfigValid = validateServoPinSetup(servoPins);
    if (bootServoPinConfigValid) {
        configureServoOutputPins(servoPins);
    } else {
        BootLogger::warn("Servo Pins", "Gecersiz pin haritasi, varsayilan cikislar korunuyor");
    }
    batteryAdcChannel = adcChannelForPin(paramManager.getPinBatteryAdc());
    baroModuleEnabled = paramManager.isBaroEnabled();
    magModuleEnabled = paramManager.isMagEnabled();
    gpsModuleEnabled = paramManager.isGpsEnabled();
    batteryModuleEnabled = paramManager.isBatteryEnabled();
#endif
#if BATTERY_ADC_ENABLED
    batteryAdc.init(
#ifdef MAVLINK_PARAMS_ENABLED
        paramManager.getPinBatteryAdc(),
#else
        PIN_BATTERY_ADC,
#endif
        batteryAdcChannel
    );
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

    gpsManager.init(nullptr, gpsModuleEnabled, GPS_UART_BAUD);
    if (gpsModuleEnabled) BootLogger::warn("GPS", "UART baglantisi bekleniyor");
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
    serviceContext.provideRcMapping = provideRcMapping;
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
    mavlink.setSensorCapabilityProvider(provideEnabledSensorCapabilities);
    mavlink.setRCOverrideEnabled(true);
    mavlink.setRCOverrideAllowedWhileArmed(false);
    mavlink.init();
#if BLACKBOX_SD_ENABLED
    blackboxSpi.begin(PIN_BLACKBOX_SPI_SCK, PIN_BLACKBOX_SPI_MISO, PIN_BLACKBOX_SPI_MOSI);
    blackbox.setSink(&blackboxSdSink);
#endif
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
    baroModuleEnabled = paramManager.isBaroEnabled();
    magModuleEnabled = paramManager.isMagEnabled();
    gpsModuleEnabled = paramManager.isGpsEnabled();
    batteryModuleEnabled = paramManager.isBatteryEnabled();
    rcModuleEnabled = paramManager.isRcEnabled();
    flightManager.setRcRequired(rcModuleEnabled && paramManager.getRcType() != 0);
#endif

    sensorCapabilities = sensorManager.capabilities();
    SensorCapabilityStatus gpsCapabilities = gpsManager.capabilities();
    const uint16_t functionMask = enabledSensorMask(sensorCapabilities.functionMask | gpsCapabilities.functionMask);
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
