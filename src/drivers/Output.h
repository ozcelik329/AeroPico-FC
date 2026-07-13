#ifndef OUTPUT_H
#define OUTPUT_H

#include <Arduino.h>
#include "board/Config.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "IDrivers.h"

class ServoOutput : public IServoOutput {
	public:
		void init() override;
		void writeMotors(int throttle, int roll, int pitch, int yaw) override;
		void setServoPulse(void* pio, unsigned sm, uint32_t pulse_us) override;
		bool isReady() const { return _ready; }

	private:
		bool _ready = false;
};

extern ServoOutput servoOutput;

// Backwards-compatible C functions
void outputInit();
void writeMotors(int throttle, int roll, int pitch, int yaw);
void setServoPulse(PIO pio, uint sm, uint32_t pulse_us);

#endif
