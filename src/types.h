#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "config.h"
// Ortak veri tipleri
struct SensorBuffer {
	float ax, ay, az;
	float gx, gy, gz;
	float tempC;
#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

struct SensorBuffer {
    float ax, ay, az;
    float gx, gy, gz;
    float tempC;
    uint32_t timestamp;
    bool valid;
};

struct FlightData {
    float roll, pitch, yaw;
    float gx, gy, gz;
    uint16_t channels[8];
    bool armed;
    bool failsafe;
};

#endif
#ifdef USE_GY87
	float mx, my, mz;
	float pressure;
#endif
	uint32_t timestamp;
	bool valid;
};
#endif