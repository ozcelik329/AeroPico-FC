#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

using SchedulerCallback = void (*)();

struct ScheduledTask {
    const char* name;
    uint16_t rateHz;
    uint32_t periodUs;
    uint32_t lastRunUs;
    uint32_t maxReleaseLatencyUs;
    uint32_t maxRuntimeUs;
    SchedulerCallback callback;
    bool enabled;
    uint32_t runCount;
    uint16_t deadlineMisses;
};

class Scheduler {
  public:
    static constexpr size_t MAX_TASKS = 16;

    void reset();
    bool addTask(const char* name, uint16_t rateHz, SchedulerCallback callback);
    void tick(uint32_t nowUs);
    size_t taskCount() const;
    const ScheduledTask* getTask(size_t index) const;
    bool setEnabled(const char* name, bool enabled);

  private:
    ScheduledTask _tasks[MAX_TASKS] = {};
    size_t _taskCount = 0;

    int findTask(const char* name) const;
};

#endif
