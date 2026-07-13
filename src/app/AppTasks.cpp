#include "AppTasks.h"

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
    handles.sensor = xTaskCreateStaticAffinitySet(
        sensorTask, "SensorTask",
        SENSOR_STACK_WORDS, NULL, 2,
        sensorTaskStack, &sensorTaskTcb,
        (1 << 0)
    );

    handles.flight = xTaskCreateStaticAffinitySet(
        flightTask, "FlightTask",
        FLIGHT_STACK_WORDS, NULL, 3,
        flightTaskStack, &flightTaskTcb,
        (1 << 1)
    );

    handles.telemetry = xTaskCreateStaticAffinitySet(
        telemetryTask, "TelemetryTask",
        TELEMETRY_STACK_WORDS, NULL, 1,
        telemetryTaskStack, &telemetryTaskTcb,
        (1 << 0)
    );

    return handles;
}
