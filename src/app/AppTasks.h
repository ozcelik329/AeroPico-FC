#ifndef APP_TASKS_H
#define APP_TASKS_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

struct AppTaskHandles {
    TaskHandle_t sensor = nullptr;
    TaskHandle_t flight = nullptr;
    TaskHandle_t telemetry = nullptr;
};

class AppTasks {
  public:
    using TaskEntry = void (*)(void*);

    static AppTaskHandles create(TaskEntry sensorTask,
                                 TaskEntry flightTask,
                                 TaskEntry telemetryTask);

  private:
    static constexpr uint16_t SENSOR_STACK_WORDS = 1536;
    static constexpr uint16_t FLIGHT_STACK_WORDS = 2048;
    static constexpr uint16_t TELEMETRY_STACK_WORDS = 1536;
};

#endif
