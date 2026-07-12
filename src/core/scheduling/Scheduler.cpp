#include "Scheduler.h"
#include "../../utils/FastMath.h"

#include <string.h>

namespace {
static uint16_t saturatingAdd(uint16_t value, uint32_t increment) {
    const uint32_t sum = (uint32_t)value + increment;
    return sum > UINT16_MAX ? UINT16_MAX : (uint16_t)sum;
}
}

void Scheduler::reset() {
    for (size_t i = 0; i < MAX_TASKS; ++i) {
        _tasks[i] = {};
    }
    _taskCount = 0;
}

bool Scheduler::addTask(const char* name, uint16_t rateHz, SchedulerCallback callback) {
    if (_taskCount >= MAX_TASKS || rateHz == 0 || callback == nullptr) {
        return false;
    }

    uint32_t periodUs = 1000000UL / rateHz;
    if (periodUs == 0) {
        return false;
    }

    _tasks[_taskCount].name = name;
    _tasks[_taskCount].rateHz = rateHz;
    _tasks[_taskCount].periodUs = periodUs;
    _tasks[_taskCount].lastRunUs = 0;
    _tasks[_taskCount].maxReleaseLatencyUs = 0;
    _tasks[_taskCount].maxRuntimeUs = 0;
    _tasks[_taskCount].callback = callback;
    _tasks[_taskCount].enabled = true;
    _tasks[_taskCount].runCount = 0;
    _tasks[_taskCount].deadlineMisses = 0;
    _taskCount++;
    return true;
}

void Scheduler::tick(uint32_t nowUs) {
    for (size_t i = 0; i < _taskCount; ++i) {
        ScheduledTask& task = _tasks[i];
        if (AEROPICO_UNLIKELY(!task.enabled)) {
            continue;
        }

        const uint32_t elapsedUs = (uint32_t)(nowUs - task.lastRunUs);
        if (AEROPICO_LIKELY(elapsedUs < task.periodUs)) {
            continue;
        }

        const uint32_t latencyUs = task.lastRunUs == 0 ? 0 : elapsedUs - task.periodUs;
        if (latencyUs > task.maxReleaseLatencyUs) {
            task.maxReleaseLatencyUs = latencyUs;
        }
        if (latencyUs >= task.periodUs) {
            task.deadlineMisses = saturatingAdd(task.deadlineMisses, latencyUs / task.periodUs);
        }
        task.lastRunUs = nowUs;
        task.runCount++;
        const uint32_t startUs = micros();
        task.callback();
        const uint32_t runtimeUs = (uint32_t)(micros() - startUs);
        if (runtimeUs > task.maxRuntimeUs) {
            task.maxRuntimeUs = runtimeUs;
        }
        if (runtimeUs > task.periodUs) {
            task.deadlineMisses = saturatingAdd(task.deadlineMisses, 1U);
        }
    }
}

size_t Scheduler::taskCount() const {
    return _taskCount;
}

const ScheduledTask* Scheduler::getTask(size_t index) const {
    if (index >= _taskCount) {
        return nullptr;
    }
    return &_tasks[index];
}

bool Scheduler::setEnabled(const char* name, bool enabled) {
    int index = findTask(name);
    if (index < 0) {
        return false;
    }
    _tasks[index].enabled = enabled;
    return true;
}

int Scheduler::findTask(const char* name) const {
    for (size_t i = 0; i < _taskCount; ++i) {
        if (_tasks[i].name && name && strcmp(_tasks[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}
