#include "Scheduler.h"
#include <string.h>

void Scheduler::reset() {
    _tasks[0] = {};
    for (size_t i = 1; i < MAX_TASKS; ++i) {
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
    _tasks[_taskCount].callback = callback;
    _tasks[_taskCount].enabled = true;
    _tasks[_taskCount].runCount = 0;
    _taskCount++;
    return true;
}

void Scheduler::tick(uint32_t nowUs) {
    for (size_t i = 0; i < _taskCount; ++i) {
        ScheduledTask& task = _tasks[i];
        if (!task.enabled) {
            continue;
        }

        if ((uint32_t)(nowUs - task.lastRunUs) < task.periodUs) {
            continue;
        }

        task.lastRunUs = nowUs;
        task.runCount++;
        task.callback();
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
