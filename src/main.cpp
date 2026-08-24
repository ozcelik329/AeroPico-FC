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
#include "core/safety/ModuleSetupRuntime.h"
#include "core/safety/PreflightRuntime.h"
#include "core/scheduling/Scheduler.h"
#include "core/safety/PreflightHealth.h"
#include "core/safety/BatteryMonitor.h"
#include "core/events/SystemEventBus.h"
#include "storage/CalibrationStorage.h"
#include "storage/ParamStorage.h"
#include "utils/Logger.h"
#include "utils/BootLogger.h"
#include "app/MavlinkServiceCommands.h"
#include "app/ServiceCommandMailbox.h"
#include "app/ServiceCommandProcessor.h"
#include "app/ConfiguratorParamRuntime.h"
#include "app/RuntimeHealthReporter.h"
#include "app/SensorBootSequence.h"
#include "telemetry/MavlinkHandler.h"
#include "telemetry/Blackbox.h"
#if BLACKBOX_SD_ENABLED
#include "hal/rp2350/RP2350_SPI.h"
#include "telemetry/BlackboxSdSink.h"
#endif
#include "app/AppTasks.h"
#include "drivers/BuzzerFeedback.h"
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
static ModuleSetupRuntime moduleSetupRuntime;
static PreflightRuntime preflightRuntime;
static RuntimeHealthReporter runtimeHealthReporter;
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
static uint32_t lastWatchdogBlockLogMs = 0;
static constexpr uint32_t CONTROL_LOOP_HZ = 1000000UL / FLIGHT_LOOP_PERIOD_US;
static bool latestBatteryCritical = false;
static bool magCalibrationActive = false;
static uint8_t batteryAdcChannel = BATTERY_ADC_CHANNEL;
static bool bootServoPinConfigValid = true;
static BuzzerFeedback buzzerFeedback;
static bool lastFeedbackArmState = false;
static bool feedbackArmStateInitialized = false;
static MavlinkServiceCommands mavlinkServiceCommands;
static ServiceCommandMailbox serviceCommandMailbox;
static ServiceCommandProcessor serviceCommandProcessor;
static TaskHandle_t sensorTaskHandle = nullptr;
static TaskHandle_t flightTaskHandle = nullptr;
static TaskHandle_t telemetryTaskHandle = nullptr;
static PreflightResult evaluatePreflight();

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

static ModuleServoPinConfig moduleServoPinsFromParams() {
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

static ServoPinConfig outputServoPinsFromModulePins(const ModuleServoPinConfig& pins) {
    return {pins.aileron, pins.elevator, pins.rudder, pins.throttle};
}

static void refreshModuleSetupFromParams() {
    ModuleSetupSnapshot snapshot = {};
    snapshot.baroEnabled = true;
    snapshot.magEnabled = true;
    snapshot.gpsEnabled = GPS_MODULE_ENABLED != 0;
    snapshot.batteryEnabled = false;
    snapshot.rcEnabled = true;
    snapshot.imuType = 1;
    snapshot.baroType = 1;
    snapshot.magType = 0;
    snapshot.gpsType = 1;
    snapshot.rcType = 1;
    snapshot.batteryType = 0;
    snapshot.bootServoPinConfigValid = bootServoPinConfigValid;
    snapshot.servoPins = moduleServoPinsFromParams();
    snapshot.i2cSda = PIN_SDA;
    snapshot.i2cScl = PIN_SCL;
    snapshot.buzzerPin = PIN_BUZZER;
#ifdef MAVLINK_PARAMS_ENABLED
    snapshot.baroEnabled = paramManager.isBaroEnabled();
    snapshot.magEnabled = paramManager.isMagEnabled();
    snapshot.gpsEnabled = paramManager.isGpsEnabled();
    snapshot.batteryEnabled = paramManager.isBatteryEnabled();
    snapshot.rcEnabled = paramManager.isRcEnabled();
    snapshot.imuType = paramManager.getImuType();
    snapshot.baroType = paramManager.getBaroType();
    snapshot.magType = paramManager.getMagType();
    snapshot.gpsType = paramManager.getGpsType();
    snapshot.rcType = paramManager.getRcType();
    snapshot.batteryType = paramManager.getBatteryType();
    snapshot.i2cSda = paramManager.getPinI2cSda();
    snapshot.i2cScl = paramManager.getPinI2cScl();
    snapshot.buzzerPin = paramManager.getPinBuzzer();
#endif
    moduleSetupRuntime.update(snapshot);
    flightManager.setRcRequired(moduleSetupRuntime.rcRequired());
}

static uint16_t enabledSensorMask(uint16_t detectedMask) {
    refreshModuleSetupFromParams();
    return moduleSetupRuntime.enabledSensorMask(detectedMask);
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
static void applyPreflightQuality(uint8_t minQuality) { preflightRuntime.setMinSensorQuality(minQuality); }
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
static PreflightResult evaluatePreflight() {
    refreshModuleSetupFromParams();
    PreflightResult result = preflightRuntime.evaluate(
        rp2040.getFreeHeap(),
        SystemTimer::outputsReady(),
        SystemTimer::checkTimingBudgets()
    );
    latestBatteryCritical = preflightRuntime.latestBatteryCritical();
    return result;
}

static void updatePreflightArmGate() {
    lastPreflightResult = evaluatePreflight();
    flightManager.setBenchForceArmAllowed(benchAdminGate.forceArmActive());
    flightManager.setPreflightArmAllowed(lastPreflightResult.canArm);
}

static void initArmStatusIndicators() {
    pinMode(PIN_ARM_LED_RED, OUTPUT);
    pinMode(PIN_ARM_LED_GREEN, OUTPUT);
    digitalWrite(PIN_ARM_LED_RED, HIGH);
    digitalWrite(PIN_ARM_LED_GREEN, LOW);
    lastFeedbackArmState = false;
    feedbackArmStateInitialized = true;
}

static void runArmStatusFeedback() {
    const bool armed = flightManager.isArmed();
    digitalWrite(PIN_ARM_LED_RED, armed ? LOW : HIGH);
    digitalWrite(PIN_ARM_LED_GREEN, armed ? HIGH : LOW);

    if (!feedbackArmStateInitialized) {
        lastFeedbackArmState = armed;
        feedbackArmStateInitialized = true;
    } else if (armed != lastFeedbackArmState) {
        if (armed) {
            buzzerFeedback.playArmMelody();
        } else {
            buzzerFeedback.playDisarmMelody();
        }
        lastFeedbackArmState = armed;
    }

    buzzerFeedback.update(millis());
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
    refreshModuleSetupFromParams();
    latestBatteryCritical = moduleSetupRuntime.batteryRequired() &&
        runtimeHealthReporter.run(lastPreflightResult, sensorTaskHandle, flightTaskHandle, telemetryTaskHandle);
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
    core0Scheduler.addTask("arm-feedback", 20000, runArmStatusFeedback);
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
    initArmStatusIndicators();
#ifdef MAVLINK_PARAMS_ENABLED
    paramManager.setArmStateProvider(provideArmState);
    paramManager.setStorage(&paramStorage);
    paramManager.init();
    refreshModuleSetupFromParams();
    const ModuleServoPinConfig moduleServoPins = moduleSetupRuntime.snapshot().servoPins;
    const ServoPinConfig servoPins = outputServoPinsFromModulePins(moduleServoPins);
    bootServoPinConfigValid = ModuleSetupRuntime::validateServoPinSetup(
        moduleServoPins,
        moduleSetupRuntime.snapshot().i2cSda,
        moduleSetupRuntime.snapshot().i2cScl,
        moduleSetupRuntime.snapshot().buzzerPin
    );
    if (bootServoPinConfigValid) {
        configureServoOutputPins(servoPins);
    } else {
        BootLogger::warn("Servo Pins", "Gecersiz pin haritasi, varsayilan cikislar korunuyor");
    }
    batteryAdcChannel = adcChannelForPin(paramManager.getPinBatteryAdc());
    sensorManager.configureI2CPins(paramManager.getPinI2cSda(), paramManager.getPinI2cScl());
    buzzerFeedback.init(paramManager.getPinBuzzer());
#else
    sensorManager.configureI2CPins(PIN_SDA, PIN_SCL);
    buzzerFeedback.init(PIN_BUZZER);
#endif
    buzzerFeedback.bootChirp();
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
    preflightRuntime.init({
        &sensorManager,
        &gpsManager,
        &rxManager,
        &batteryMonitor,
        &moduleSetupRuntime,
        &preflightHealth
    });

    const bool imuOk = SensorBootSequence::run(sensorManager, calibrationStorage);

    refreshModuleSetupFromParams();
    const bool gpsEnabled = moduleSetupRuntime.snapshot().gpsEnabled;
    gpsManager.init(nullptr, gpsEnabled, GPS_UART_BAUD);
    if (gpsEnabled) BootLogger::warn("GPS", "UART baglantisi bekleniyor");
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
    runtimeHealthReporter.init({
        &batteryMonitor,
        &blackbox,
        &mavlink,
        &sensorManager,
        &systemEventBus
    });
#ifdef MAVLINK_PARAMS_ENABLED
    ConfiguratorParamRuntime::bindAndApply(paramManager,
                                           applyPidGains,
                                           applyMixerSettings,
                                           applyFailsafeTimeout,
                                           applyRcMapping,
                                           applyMavlinkRates,
                                           applyBlackboxRate,
                                           applyPreflightQuality,
                                           applyBatteryProfile);
    refreshModuleSetupFromParams();
#endif

    SensorCapabilityStatus sensorCapabilities = sensorManager.capabilities();
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
