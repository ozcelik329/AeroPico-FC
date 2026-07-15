#include "AppTasks.h"

namespace {
constexpr UBaseType_t kCore0Affinity = (1u << 0);
constexpr UBaseType_t kCore1Affinity = (1u << 1);

TaskHandle_t createPinnedTask(AppTasks::TaskEntry entry,
                              const char* name,
                              uint32_t stackDepthWords,
                              UBaseType_t priority,
                              StackType_t* stack,
                              StaticTask_t* tcb,
                              UBaseType_t preferredAffinity,
                              UBaseType_t fallbackAffinity) {
    TaskHandle_t handle = xTaskCreateStaticAffinitySet(
        entry, name, stackDepthWords, nullptr, priority,
        stack, tcb, preferredAffinity
    );

    if (!handle && fallbackAffinity != preferredAffinity) {
        handle = xTaskCreateStaticAffinitySet(
            entry, name, stackDepthWords, nullptr, priority,
            stack, tcb, fallbackAffinity
        );
    }

    return handle;
}
}

AppTaskHandles AppTasks::create(TaskEntry sensorTask,
                                TaskEntry flightTask,
                                TaskEntry telemetryTask) {
    static StaticTask_t sensorTaskTcb;
    static StaticTask_t flightTaskTcb;
    static StaticTask_t telemetryTaskTcb;
    static StackType_t sensorTaskStack[SENSOR_STACK_WORDS];
    static StackType_t flightTaskStack[FLIGHT_STACK_WORDS];
    static StackType_t telemetryTaskStack[TELEMETRY_STACK_WORDS];

    AppTaskHandles handles;
    handles.sensor = createPinnedTask(sensorTask, "SensorTask",
                                      SENSOR_STACK_WORDS, 2,
                                      sensorTaskStack, &sensorTaskTcb,
                                      kCore0Affinity, kCore0Affinity);

    handles.flight = createPinnedTask(flightTask, "FlightTask",
                                      FLIGHT_STACK_WORDS, 3,
                                      flightTaskStack, &flightTaskTcb,
                                      kCore0Affinity, kCore0Affinity);

    handles.telemetry = createPinnedTask(telemetryTask, "TelemetryTask",
                                         TELEMETRY_STACK_WORDS, 1,
                                         telemetryTaskStack, &telemetryTaskTcb,
                                         kCore0Affinity, kCore0Affinity);

    return handles;
}
