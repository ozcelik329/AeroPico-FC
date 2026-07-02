#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "config.h"
// Ortak veri tipleri
struct SensorBuffer {
	float ax, ay, az;
	float gx, gy, gz;
	float tempC;
#ifdef USE_GY87
	float mx, my, mz;
	float pressure;
#endif
	uint32_t timestamp;
	bool valid;
};
#endif