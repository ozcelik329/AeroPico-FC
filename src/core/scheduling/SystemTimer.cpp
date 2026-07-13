#include "SystemTimer.h"
#include "../control/FlightControlTask.h"

volatile bool SystemManager::is_running = false;
volatile uint32_t SystemManager::core1HeartbeatUs = 0;

uint32_t SystemManager::getCore1HeartbeatUs() {
    __dmb();
    return core1HeartbeatUs;
}

void SystemTimer::initTimingMeasurements() {
    FlightControlTask::initTimingMeasurements();
}

void SystemTimer::beginTiming(LoopPhase phase) {
    FlightControlTask::beginTiming(phase);
}

void SystemTimer::endTiming(LoopPhase phase) {
    FlightControlTask::endTiming(phase);
}

bool SystemTimer::checkTimingBudgets() {
    return FlightControlTask::checkTimingBudgets();
}

TimingBudgetStatus SystemTimer::getTimingBudgetStatus() {
    return FlightControlTask::getTimingBudgetStatus();
}

void SystemTimer::requestTimingWindowReset() {
    FlightControlTask::requestTimingWindowReset();
}

bool SystemTimer::outputsReady() {
    return FlightControlTask::outputsReady();
}

bool SystemTimer::requestServoTest(uint8_t surface, uint16_t pulseUs, uint16_t durationMs) {
    return FlightControlTask::requestServoTest(surface, pulseUs, durationMs);
}

void SystemTimer::applyPidGains(float angleP, float angleI, float angleD,
                                float rateP, float rateI, float rateD) {
    FlightControlTask::applyPidGains(angleP, angleI, angleD, rateP, rateI, rateD);
}

void SystemTimer::applyMixerSettings(const MixerSettings& settings) {
    FlightControlTask::applyMixerSettings(settings);
}

void SystemManager::init() {
    FlightControlTask::init();
}

void SystemManager::core1_entry() {
    FlightControlTask::run();
}
