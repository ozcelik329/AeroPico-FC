#ifndef IDRIVERS_H
#define IDRIVERS_H

#include "../types.h"

class IImuDriver {
  public:
    virtual ~IImuDriver() {}
    virtual void init() = 0;
    virtual void update() = 0;
    virtual SensorBuffer getLatest() = 0;
};

class IMagDriver {
  public:
    virtual ~IMagDriver() {}
    virtual bool hasMag() const = 0;
    virtual void init() = 0;
    virtual void update() = 0;
};

class IBaroDriver {
  public:
    virtual ~IBaroDriver() {}
    virtual bool hasBaro() const = 0;
    virtual void init() = 0;
    virtual void update() = 0;
};

class IGpsDriver {
  public:
    virtual ~IGpsDriver() {}
    virtual void init() = 0;
    virtual void update() = 0;
};

class IServoOutput {
  public:
    virtual ~IServoOutput() {}
    virtual void init() = 0;
    virtual void writeMotors(int throttle, int roll, int pitch, int yaw) = 0;
    virtual void setServoPulse(void* pio, unsigned sm, uint32_t pulse_us) = 0;
};

class IRxDriver {
  public:
    virtual ~IRxDriver() {}
    virtual void init() = 0;
    virtual void update() = 0;
    virtual bool isValid() const = 0;
    virtual bool isFailsafe() const = 0;
    virtual uint32_t lastValidMs() const = 0;
    virtual uint16_t getChannel(int ch) const = 0;
};

#endif
