#ifndef SBUS_BACKEND_H
#define SBUS_BACKEND_H

#include "SbusMapper.h"

class ISbusBackend {
  public:
    virtual ~ISbusBackend() {}
    virtual void begin() = 0;
    virtual bool readFrame(SbusFrameView& frame) = 0;
};

#endif
