#include "FlightControlTask.h"
#include "ControlLoopExecutor.h"
#include "../flight/FlightManager.h"
#include "../scheduling/TimingMonitor.h"
#include "board/Config.h"
#include "../../def.h"
#include "../../hal/rp2350/RP2350_GPIO.h"
#include "../../hal/rp2350/RP2350_PWM.h"
#include "../../hal/rp2350/RP2350_Timer.h"
#include <FreeRTOS.h>
#include <task.h>

static TimingMonitor timingMonitor;
static RP2350GPIO gpioBackend;
static RP2350PWM pwmOutput;
static RP2350Timer timerBackend;
static ControlLoopExecutor controlLoop;
static volatile bool timingWindowResetRequested = false;

struct PendingPidGains {
    float angleP;
    float angleI;
    float angleD;
    float rateP;
    float rateI;
    float rateD;
};

static PendingPidGains pendingPidGains = {};
static MixerSettings pendingMixerSettings = {};
static uint32_t pendingPidSeq = 0;
static uint32_t appliedPidSeq = 0;
static uint32_t pendingMixerSeq = 0;
static uint32_t appliedMixerSeq = 0;
static volatile uint32_t servoTestUntilUs = 0;
static volatile uint16_t servoTestPulseUs = PWM_NEUTRAL;
static volatile uint8_t servoTestSurface = 0;

static void publishPendingPidGains(const PendingPidGains& gains) {
    uint32_t next = __atomic_load_n(&pendingPidSeq, __ATOMIC_RELAXED) + 1U;
    if ((next & 1U) == 0U) {
        ++next;
    }
    __atomic_store_n(&pendingPidSeq, next, __ATOMIC_RELEASE);
    pendingPidGains = gains;
    __atomic_store_n(&pendingPidSeq, next + 1U, __ATOMIC_RELEASE);
}

static void publishPendingMixerSettings(const MixerSettings& settings) {
    uint32_t next = __atomic_load_n(&pendingMixerSeq, __ATOMIC_RELAXED) + 1U;
    if ((next & 1U) == 0U) {
        ++next;
    }
    __atomic_store_n(&pendingMixerSeq, next, __ATOMIC_RELEASE);
    pendingMixerSettings = settings;
    __atomic_store_n(&pendingMixerSeq, next + 1U, __ATOMIC_RELEASE);
}

static void applyPendingControlSettings() {
    uint32_t pidSeq1 = __atomic_load_n(&pendingPidSeq, __ATOMIC_ACQUIRE);
    if ((pidSeq1 & 1U) == 0U && pidSeq1 != appliedPidSeq) {
        PendingPidGains gains = pendingPidGains;
        uint32_t pidSeq2 = __atomic_load_n(&pendingPidSeq, __ATOMIC_ACQUIRE);
        if (pidSeq1 == pidSeq2) {
            controlLoop.applyPidGains(gains.angleP, gains.angleI, gains.angleD,
                                      gains.rateP, gains.rateI, gains.rateD);
            appliedPidSeq = pidSeq2;
        }
    }

    uint32_t mixerSeq1 = __atomic_load_n(&pendingMixerSeq, __ATOMIC_ACQUIRE);
    if ((mixerSeq1 & 1U) == 0U && mixerSeq1 != appliedMixerSeq) {
        MixerSettings settings = pendingMixerSettings;
        uint32_t mixerSeq2 = __atomic_load_n(&pendingMixerSeq, __ATOMIC_ACQUIRE);
        if (mixerSeq1 == mixerSeq2) {
            controlLoop.applyMixerSettings(settings);
            appliedMixerSeq = mixerSeq2;
        }
    }
}

void FlightControlTask::initTimingMeasurements() {
    timingMonitor.init(&timerBackend, &gpioBackend);

    gpioBackend.configure(PIN_DEBUG_CONSUME, HALGpioMode::Output);
    gpioBackend.write(PIN_DEBUG_CONSUME, false);

    gpioBackend.configure(PIN_DEBUG_PID, HALGpioMode::Output);
    gpioBackend.write(PIN_DEBUG_PID, false);

    gpioBackend.configure(PIN_DEBUG_MIXER, HALGpioMode::Output);
    gpioBackend.write(PIN_DEBUG_MIXER, false);
}

void FlightControlTask::beginTiming(SystemTimer::LoopPhase phase) {
    timingMonitor.begin(phase);
    switch (phase) {
        case SystemTimer::PHASE_CONSUME:
            gpioBackend.write(PIN_DEBUG_CONSUME, true);
            break;
        case SystemTimer::PHASE_PID:
            gpioBackend.write(PIN_DEBUG_PID, true);
            break;
        case SystemTimer::PHASE_MIXER:
            gpioBackend.write(PIN_DEBUG_MIXER, true);
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

void FlightControlTask::requestTimingWindowReset() {
    __atomic_store_n(&timingWindowResetRequested, true, __ATOMIC_RELEASE);
}

bool FlightControlTask::outputsReady() {
    return controlLoop.outputsReady();
}

bool FlightControlTask::requestServoTest(uint8_t surface, uint16_t pulseUs, uint16_t durationMs) {
    if (!controlLoop.outputsReady()) {
        return false;
    }
    const uint16_t safeDurationMs = constrain(durationMs, (uint16_t)100, (uint16_t)1500);
    const uint16_t safePulseUs = constrain(pulseUs, (uint16_t)PWM_MIN, (uint16_t)PWM_MAX);
    __atomic_store_n(&servoTestSurface, surface, __ATOMIC_RELEASE);
    __atomic_store_n(&servoTestPulseUs, safePulseUs, __ATOMIC_RELEASE);
    __atomic_store_n(&servoTestUntilUs, micros() + (uint32_t)safeDurationMs * 1000U, __ATOMIC_RELEASE);
    return true;
}

void FlightControlTask::applyPidGains(float angleP, float angleI, float angleD,
                                      float rateP, float rateI, float rateD) {
    publishPendingPidGains({angleP, angleI, angleD, rateP, rateI, rateD});
}

void FlightControlTask::applyMixerSettings(const MixerSettings& settings) {
    publishPendingMixerSettings(settings);
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
        if (__atomic_exchange_n(&timingWindowResetRequested, false, __ATOMIC_ACQ_REL)) {
            timingMonitor.resetWindow();
        }
        applyPendingControlSettings();
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
            const uint32_t testUntil = __atomic_load_n(&servoTestUntilUs, __ATOMIC_ACQUIRE);
            if ((int32_t)(testUntil - now) > 0) {
                controlLoop.writeServoTestOutputs(
                    __atomic_load_n(&servoTestSurface, __ATOMIC_ACQUIRE),
                    __atomic_load_n(&servoTestPulseUs, __ATOMIC_ACQUIRE)
                );
            } else {
                controlLoop.writeSafeOutputs();
            }
            endTiming(SystemTimer::PHASE_TOTAL);
            vTaskDelayUntil(&nextWake, loopPeriodTicks);
            continue;
        }

        beginTiming(SystemTimer::PHASE_CONSUME);
        FlightData controlData = {};
        const bool hasControlData = flightManager.consumeLatest(controlData);
        endTiming(SystemTimer::PHASE_CONSUME);
        if (!hasControlData) {
            controlLoop.writeSafeOutputs();
            endTiming(SystemTimer::PHASE_TOTAL);
            vTaskDelayUntil(&nextWake, loopPeriodTicks);
            continue;
        }

        ControlCorrections corrections = {};
        beginTiming(SystemTimer::PHASE_PID);
        if (controlData.controlMode == ControlMode::Stabilize) {
            corrections = controlLoop.computeCorrections(controlData, dt);
        }
        endTiming(SystemTimer::PHASE_PID);

        beginTiming(SystemTimer::PHASE_MIXER);
        controlLoop.mixAndWrite(controlData, corrections);
        endTiming(SystemTimer::PHASE_MIXER);

        endTiming(SystemTimer::PHASE_TOTAL);
        vTaskDelayUntil(&nextWake, loopPeriodTicks);
    }
}
