#ifndef NAVIGATION_CONTROLLER_H
#define NAVIGATION_CONTROLLER_H

#include <Arduino.h>
#include "../drivers/RX.h"

class NavigationController {
  public:
    void init();
    void update(uint16_t aileron, uint16_t elevator, bool failsafe);

  private:
    // Placeholder for navigation-specific state (e.g., autopilot targets)
    bool _active = false;
};

#endif

