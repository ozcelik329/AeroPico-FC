#ifndef SENSOR_BACKEND_REGISTRY_H
#define SENSOR_BACKEND_REGISTRY_H

#include "SensorDeviceProfile.h"

class SensorBackendRegistry {
  public:
    static const MagDeviceProfile& hmc5883l() {
        static const MagDeviceProfile profile = {
            0x1E,
            0x00, 0x70,
            0x01, 0xA0,
            0x02, 0x00,
            0x03,
            6
        };
        return profile;
    }

    static const BaroDeviceProfile& bmp085() {
        static const BaroDeviceProfile profile = {
            0x77,
            0xAA,
            22,
            0xF4,
            0xF6,
            0x2E,
            0x34,
            3,
            5000,
            26000
        };
        return profile;
    }
};

#endif
