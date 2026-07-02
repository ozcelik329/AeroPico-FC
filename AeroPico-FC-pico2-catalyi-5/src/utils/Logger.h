#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

class Logger {
  public:
    static void init();
    static void log(const char* message);
    static void logError(const char* error);
};

#endif