#ifndef SERVICE_COMMAND_PROCESSOR_H
#define SERVICE_COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../drivers/Sensors.h"
#include "../storage/CalibrationStorage.h"
#include "ServiceCommandMailbox.h"

struct ServiceCommandProcessorContext {
    SensorManager* sensors = nullptr;
    ICalibrationStorage* calibrationStorage = nullptr;
    bool* magCalibrationActive = nullptr;
    bool (*isArmed)() = nullptr;
    bool (*requestServoTest)(uint8_t surface, uint16_t pulseUs, uint16_t durationMs) = nullptr;
    ServiceCommandMailbox* mailbox = nullptr;
};

class ServiceCommandProcessor {
  public:
    void init(const ServiceCommandProcessorContext& context);
    void process();

  private:
    ServiceCommandProcessorContext _context = {};

    void complete(uint16_t action, uint8_t result, const char* reason);
    void pollAsyncCompletions();
    void processRequest(const ServiceCommandRequest& request);
    bool rejectIfArmed(uint16_t action, const char* reason);
};

#endif
