#include "SystemTimer.h"
#include "FlightManager.h"
#include "FixedWingMixer.h"
#include "../drivers/Output.h"
#include "../def.h"
#include "../config.h"
#include "PID.h"
#include <pico/time.h>
#include <hardware/gpio.h>

static PID rollAnglePID(ANGLE_P_GAIN, ANGLE_I_GAIN, ANGLE_D_GAIN);
static PID pitchAnglePID(ANGLE_P_GAIN, ANGLE_I_GAIN, ANGLE_D_GAIN);
static PID rollRatePID(RATE_P_GAIN, RATE_I_GAIN, RATE_D_GAIN);
static PID pitchRatePID(RATE_P_GAIN, RATE_I_GAIN, RATE_D_GAIN);
static PID yawRatePID(RATE_P_GAIN, RATE_I_GAIN, RATE_D_GAIN);
static FixedWingMixer mixer;

static volatile uint32_t phaseStartUs[SystemTimer::PHASE_COUNT];
static volatile uint32_t phaseMaxUs[SystemTimer::PHASE_COUNT];
static volatile bool phaseBudgetExceeded[SystemTimer::PHASE_COUNT];

volatile bool SystemManager::is_running = false;
volatile uint32_t SystemManager::core1HeartbeatUs = 0;

uint32_t SystemManager::getCore1HeartbeatUs() {
    __dmb();
    return core1HeartbeatUs;
}

void SystemTimer::initTimingMeasurements() {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        phaseStartUs[i] = 0;
        phaseMaxUs[i] = 0;
        phaseBudgetExceeded[i] = false;
    }

    gpio_init(PIN_DEBUG_CONSUME);
    gpio_set_dir(PIN_DEBUG_CONSUME, GPIO_OUT);
    gpio_put(PIN_DEBUG_CONSUME, 0);

    gpio_init(PIN_DEBUG_PID);
    gpio_set_dir(PIN_DEBUG_PID, GPIO_OUT);
    gpio_put(PIN_DEBUG_PID, 0);

    gpio_init(PIN_DEBUG_MIXER);
    gpio_set_dir(PIN_DEBUG_MIXER, GPIO_OUT);
    gpio_put(PIN_DEBUG_MIXER, 0);
}

void SystemTimer::beginTiming(LoopPhase phase) {
    phaseStartUs[phase] = time_us_32();
    switch (phase) {
        case PHASE_CONSUME:
            gpio_put(PIN_DEBUG_CONSUME, 1);
            break;
        case PHASE_PID:
            gpio_put(PIN_DEBUG_PID, 1);
            break;
        case PHASE_MIXER:
            gpio_put(PIN_DEBUG_MIXER, 1);
            break;
        default:
            break;
    }
}

void SystemTimer::endTiming(LoopPhase phase) {
    uint32_t now = time_us_32();
    uint32_t elapsed = now - phaseStartUs[phase];
    if (elapsed > phaseMaxUs[phase]) {
        phaseMaxUs[phase] = elapsed;
    }

    switch (phase) {
        case PHASE_CONSUME:
            gpio_put(PIN_DEBUG_CONSUME, 0);
            if (elapsed > PHASE_CONSUME_BUDGET_US) phaseBudgetExceeded[phase] = true;
            break;
        case PHASE_PID:
            gpio_put(PIN_DEBUG_PID, 0);
            if (elapsed > PHASE_PID_BUDGET_US) phaseBudgetExceeded[phase] = true;
            break;
        case PHASE_MIXER:
            gpio_put(PIN_DEBUG_MIXER, 0);
            if (elapsed > PHASE_MIXER_BUDGET_US) phaseBudgetExceeded[phase] = true;
            break;
        case PHASE_TOTAL:
            if (elapsed > PHASE_TOTAL_BUDGET_US) phaseBudgetExceeded[phase] = true;
            break;
        default:
            break;
    }
}

bool SystemTimer::checkTimingBudgets() {
    for (size_t i = 0; i < PHASE_COUNT; ++i) {
        if (phaseBudgetExceeded[i]) {
            return false;
        }
    }
    return true;
}

void SystemTimer::logTimingStats() {
    Serial.printf("Timing budgets: consume=%u/%uus pid=%u/%uus mixer=%u/%uus total=%u/%uus\n",
        phaseMaxUs[PHASE_CONSUME], PHASE_CONSUME_BUDGET_US,
        phaseMaxUs[PHASE_PID], PHASE_PID_BUDGET_US,
        phaseMaxUs[PHASE_MIXER], PHASE_MIXER_BUDGET_US,
        phaseMaxUs[PHASE_TOTAL], PHASE_TOTAL_BUDGET_US);
}

static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void SystemManager::init() {
    // Output sadece burada başlatılıyor (FixedWingMixer::init() çağrılmıyor)
    outputInit();

    MixerSettings settings;
    settings.rollGain  = 1.0f;
    settings.pitchGain = 1.0f;
    settings.yawGain   = 0.8f;
    settings.aileronTrim  = 0;
    settings.elevatorTrim = 0;
    settings.rudderTrim   = 0;
    settings.throttleTrim = 0;
    mixer.setSettings(settings);

    rollAnglePID.reset();
    pitchAnglePID.reset();
    rollRatePID.reset();
    pitchRatePID.reset();
    yawRatePID.reset();

    initTimingMeasurements();
    is_running = true;
    core1HeartbeatUs = micros();
    multicore_launch_core1(core1_entry);
}

void SystemManager::core1_entry() {
    // Sabit frekanslı döngü: LOOP_TIME µs = 500 Hz
    uint32_t next_tick = micros();

    while (true) {
        uint32_t now = micros();

        // Zamanlanmış tick'i bekle (busy-wait yerine sleep)
        if ((int32_t)(now - next_tick) < 0) {
            continue;
        }

        // Heartbeat: armed/disarmed farketmeksizin HER turda güncellenir.
        // Core 0 bunu okuyup Core 1'in canlı olduğunu doğrular.
        core1HeartbeatUs = now;
        __dmb();

        beginTiming(PHASE_TOTAL);

        // Gerçek dt: bir önceki tick'ten bu yana geçen süre
        float dt = (float)LOOP_TIME / 1000000.0f;
        next_tick += LOOP_TIME;
        if (!flightManager.isArmed()) {
            writeMotors(PWM_MIN, PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL);
            endTiming(PHASE_TOTAL);
            continue;
        }

        // Tüketici: buffer'dan yeni örnekleri al (SINGLE consumer - Core 1)
        beginTiming(PHASE_CONSUME);
        flightManager.consumeLatest();
        endTiming(PHASE_CONSUME);

        // RC girişlerini aç sınıra çevir
        beginTiming(PHASE_PID);
        float targetRoll    = mapFloat(flightManager.getAileron(),  1000.0f, 2000.0f, -MAX_ROLL_ANGLE,  MAX_ROLL_ANGLE);
        float targetPitch   = mapFloat(flightManager.getElevator(), 1000.0f, 2000.0f, -MAX_PITCH_ANGLE, MAX_PITCH_ANGLE);
        float targetYawRate = mapFloat(flightManager.getRudder(),   1000.0f, 2000.0f, -MAX_YAW_RATE,    MAX_YAW_RATE);

        // Cascaded PID: açı → açısal hız
        float desiredRollRate  = rollAnglePID.compute(targetRoll,  flightManager.getRoll(),  dt);
        float desiredPitchRate = pitchAnglePID.compute(targetPitch, flightManager.getPitch(), dt);

        // Açısal hız → servo düzeltmesi
        float rollCorr  = rollRatePID.compute(desiredRollRate,  flightManager.getGyroX(), dt);
        float pitchCorr = pitchRatePID.compute(desiredPitchRate, flightManager.getGyroY(), dt);
        float yawCorr   = yawRatePID.compute(targetYawRate,     flightManager.getGyroZ(), dt);
        endTiming(PHASE_PID);

        beginTiming(PHASE_MIXER);
        mixer.compute(
            flightManager.getThrottle(),
            rollCorr,
            pitchCorr,
            yawCorr,
            flightManager.getAileron(),
            flightManager.getElevator(),
            flightManager.getRudder()
        );
        endTiming(PHASE_MIXER);

        endTiming(PHASE_TOTAL);
    }
}