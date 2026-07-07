#include "FlightControlTask.h"
#include "ControlLoopExecutor.h"
#include "../FlightManager.h"
#include "../TimingMonitor.h"
#include "../../config.h"
#include "../../def.h"
#include "../../hal/rp2350/RP2350_PWM.h"
#include <FreeRTOS.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include <task.h>

static TimingMonitor timingMonitor;
static RP2350PWM pwmOutput;
static ControlLoopExecutor controlLoop;

void FlightControlTask::initTimingMeasurements() {
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

void FlightControlTask::beginTiming(SystemTimer::LoopPhase phase) {
    timingMonitor.begin(phase);
    switch (phase) {
        case SystemTimer::PHASE_CONSUME:
            gpio_put(PIN_DEBUG_CONSUME, 1);
            break;
        case SystemTimer::PHASE_PID:
            gpio_put(PIN_DEBUG_PID, 1);
            break;
        case SystemTimer::PHASE_MIXER:
            gpio_put(PIN_DEBUG_MIXER, 1);
            break;
        default:
            break;
    }
}

void FlightControlTask::endTiming(SystemTimer::LoopPhase phase) {
    switch (phase) {
        case SystemTimer::PHASE_CONSUME:
            timingMonitor.end(phase, SystemTimer::PHASE_CONSUME_BUDGET_US, PIN_DEBUG_CONSUME);
            break;
        case SystemTimer::PHASE_PID:
            timingMonitor.end(phase, SystemTimer::PHASE_PID_BUDGET_US, PIN_DEBUG_PID);
            break;
        case SystemTimer::PHASE_MIXER:
            timingMonitor.end(phase, SystemTimer::PHASE_MIXER_BUDGET_US, PIN_DEBUG_MIXER);
            break;
        case SystemTimer::PHASE_TOTAL:
            timingMonitor.end(phase, SystemTimer::PHASE_TOTAL_BUDGET_US);
            break;
        default:
            break;
    }
}

bool FlightControlTask::checkTimingBudgets() {
    return timingMonitor.checkBudgets();
}

TimingBudgetStatus FlightControlTask::getTimingBudgetStatus() {
    return timingMonitor.getStatus();
}

void FlightControlTask::logTimingStats() {
    timingMonitor.logStats(SystemTimer::PHASE_CONSUME_BUDGET_US,
                           SystemTimer::PHASE_PID_BUDGET_US,
                           SystemTimer::PHASE_MIXER_BUDGET_US,
                           SystemTimer::PHASE_TOTAL_BUDGET_US);
}

bool FlightControlTask::outputsReady() {
    return controlLoop.outputsReady();
}

void FlightControlTask::applyPidGains(float angleP, float angleI, float angleD,
                                      float rateP, float rateI, float rateD) {
    controlLoop.applyPidGains(angleP, angleI, angleD, rateP, rateI, rateD);
}

void FlightControlTask::applyMixerSettings(const MixerSettings& settings) {
    controlLoop.applyMixerSettings(settings);
}

void FlightControlTask::init() {
    controlLoop.init(&pwmOutput);
    initTimingMeasurements();
    SystemTimer::is_running = true;
    SystemTimer::core1HeartbeatUs = micros();
}

void FlightControlTask::run() {
    const TickType_t loopPeriodTicks = pdMS_TO_TICKS(SystemTimer::LOOP_TIME_US / 1000);
    TickType_t nextWake = xTaskGetTickCount();
    uint32_t lastTickUs = micros();

    while (true) {
        uint32_t now = micros();

        SystemTimer::core1HeartbeatUs = now;
        __dmb();

        beginTiming(SystemTimer::PHASE_TOTAL);

        float dt = (float)(now - lastTickUs) / 1000000.0f;
        lastTickUs = now;
        if (dt <= 0.0f || dt > 0.05f) {
            dt = (float)SystemTimer::LOOP_TIME_US / 1000000.0f;
        }

        if (!flightManager.isArmed()) {
            controlLoop.writeSafeOutputs();
            endTiming(SystemTimer::PHASE_TOTAL);
            vTaskDelayUntil(&nextWake, loopPeriodTicks);
            continue;
        }

        beginTiming(SystemTimer::PHASE_CONSUME);
        flightManager.consumeLatest();
        endTiming(SystemTimer::PHASE_CONSUME);

        beginTiming(SystemTimer::PHASE_PID);
        ControlCorrections corrections = controlLoop.computeCorrections(flightManager, dt);
        endTiming(SystemTimer::PHASE_PID);

        beginTiming(SystemTimer::PHASE_MIXER);
        controlLoop.mixAndWrite(flightManager, corrections);
        endTiming(SystemTimer::PHASE_MIXER);

        endTiming(SystemTimer::PHASE_TOTAL);
        vTaskDelayUntil(&nextWake, loopPeriodTicks);
    }
}
