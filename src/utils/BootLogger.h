#ifndef BOOTLOGGER_H
#define BOOTLOGGER_H

#include <Arduino.h>

class BootLogger {
  public:
    static void printBanner();
    static void printReadyMessage();
    static void printHealthReport(
        uint32_t loopRateHz,
        bool imuOk,
        bool baroOk,
        bool magOk,
        bool receiverOk,
        bool dmaOk,
        bool armed,
        bool failsafe,
        uint32_t heapBytes
    );
    static void ok(const char* name);
    static void okWithValue(const char* name, const char* value);
    static void fail(const char* name, const char* reason);
    static void warn(const char* name, const char* reason);
    static void info(const char* text);
};

#endif
