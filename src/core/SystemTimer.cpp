#include "SystemTimer.h"
#include "FlightManager.h"
#include "TimingMonitor.h"
#include "control/ControlLoopExecutor.h"
#include "../hal/rp2350/RP2350_PWM.h"
#include "../def.h"
#include "../config.h"
#include <FreeRTOS.h>
#include <task.h>
#include <pico/time.h>
#include <hardware/gpio.h>

static TimingMonitor timingMonitor;
static RP2350PWM pwmOutput;
static ControlLoopExecutor controlLoop;

volatile bool SystemManager::is_running = false;
volatile uint32_t SystemManager::core1HeartbeatUs = 0;

uint32_t SystemManager::getCore1HeartbeatUs() {
    __dmb();
    return core1HeartbeatUs;
}

void SystemTimer::initTimingMeasurements() {
    timingMonitor.init();

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
    timingMonitor.begin(phase);
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
    switch (phase) {
        case PHASE_CONSUME:
            timingMonitor.end(phase, PHASE_CONSUME_BUDGET_US, PIN_DEBUG_CONSUME);
            break;
        case PHASE_PID:
            timingMonitor.end(phase, PHASE_PID_BUDGET_US, PIN_DEBUG_PID);
            break;
        case PHASE_MIXER:
            timingMonitor.end(phase, PHASE_MIXER_BUDGET_US, PIN_DEBUG_MIXER);
            break;
        case PHASE_TOTAL:
            timingMonitor.end(phase, PHASE_TOTAL_BUDGET_US);
            break;
        default:
            break;
    }
}

bool SystemTimer::checkTimingBudgets() {
    return timingMonitor.checkBudgets();
}

TimingBudgetStatus SystemTimer::getTimingBudgetStatus() {
    return timingMonitor.getStatus();
}

void SystemTimer::logTimingStats() {
    timingMonitor.logStats(PHASE_CONSUME_BUDGET_US,
                           PHASE_PID_BUDGET_US,
                           PHASE_MIXER_BUDGET_US,
                           PHASE_TOTAL_BUDGET_US);
}

bool SystemTimer::outputsReady() {
    return controlLoop.outputsReady();
}

void SystemTimer::applyPidGains(float angleP, float angleI, float angleD,
                                float rateP, float rateI, float rateD) {
    controlLoop.applyPidGains(angleP, angleI, angleD, rateP, rateI, rateD);
}

void SystemTimer::applyMixerSettings(const MixerSettings& settings) {
    controlLoop.applyMixerSettings(settings);
}

void SystemManager::init() {
    controlLoop.init(&pwmOutput);
    initTimingMeasurements();
    is_running = true;
    core1HeartbeatUs = micros();
}

void SystemManager::core1_entry() {
    const TickType_t loopPeriodTicks = pdMS_TO_TICKS(LOOP_TIME_US / 1000);
    TickType_t nextWake = xTaskGetTickCount();
    uint32_t lastTickUs = micros();

    while (true) {
        uint32_t now = micros();

        // Heartbeat: armed/disarmed farketmeksizin HER turda güncellenir.
        // Core 0 bunu okuyup Core 1'in canlı olduğunu doğrular.
        core1HeartbeatUs = now;
        __dmb();

        beginTiming(PHASE_TOTAL);

        float dt = (float)(now - lastTickUs) / 1000000.0f;
        lastTickUs = now;
        if (dt <= 0.0f || dt > 0.05f) {
            dt = (float)LOOP_TIME_US / 1000000.0f;
        }

        if (!flightManager.isArmed()) {
            controlLoop.writeSafeOutputs();
            endTiming(PHASE_TOTAL);
            vTaskDelayUntil(&nextWake, loopPeriodTicks);
            continue;
        }

        // Tüketici: buffer'dan yeni örnekleri al (SINGLE consumer - Core 1)
        beginTiming(PHASE_CONSUME);
        flightManager.consumeLatest();
        endTiming(PHASE_CONSUME);

        beginTiming(PHASE_PID);
        ControlCorrections corrections = controlLoop.computeCorrections(flightManager, dt);
        endTiming(PHASE_PID);

        beginTiming(PHASE_MIXER);
        controlLoop.mixAndWrite(flightManager, corrections);
        endTiming(PHASE_MIXER);

        endTiming(PHASE_TOTAL);
        vTaskDelayUntil(&nextWake, loopPeriodTicks);
    }
}
