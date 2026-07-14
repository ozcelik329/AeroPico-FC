#ifndef SERVICE_COMMAND_MAILBOX_H
#define SERVICE_COMMAND_MAILBOX_H

#include <Arduino.h>

struct ServiceCommandRequest {
    uint16_t action;
    float p2;
    float p3;
    float p4;
};

struct ServiceCommandCompletion {
    uint16_t action;
    uint8_t result;
    char reason[72];
};

class ServiceCommandMailbox {
  public:
    bool submit(const ServiceCommandRequest& request);
    bool claim(ServiceCommandRequest& request);
    void complete(uint16_t action, uint8_t result, const char* reason);
    bool takeCompletion(ServiceCommandCompletion& completion);

  private:
    ServiceCommandRequest _pending = {};
    ServiceCommandCompletion _completion = {};
    uint8_t _pendingReady = 0;
    uint8_t _completionReady = 0;
};

#endif
