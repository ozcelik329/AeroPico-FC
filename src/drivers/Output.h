#ifndef OUTPUT_H
#define OUTPUT_H

#include <Arduino.h>
#include "../config.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

void outputInit();
void writeMotors(int throttle, int roll, int pitch, int yaw);
void setServoPulse(PIO pio, uint sm, uint32_t pulse_us);

#endif