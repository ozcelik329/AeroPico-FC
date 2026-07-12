#ifndef GPS_TYPES_H
#define GPS_TYPES_H

#include <stdint.h>

struct GpsFix {
    int32_t latitudeE7;
    int32_t longitudeE7;
    float altitudeM;
    uint16_t hdopCm;
    uint8_t satellites;
    bool valid;
};

struct GpsStatus {
    GpsFix fix;
    uint32_t lastFixMs;
    uint32_t sentences;
    uint32_t checksumErrors;
    bool enabled;
};

#endif
