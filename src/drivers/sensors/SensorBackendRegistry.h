#ifndef SENSOR_BACKEND_REGISTRY_H
#define SENSOR_BACKEND_REGISTRY_H

#include "SensorDeviceProfile.h"

class SensorBackendRegistry {
  public:
    static const ImuDeviceProfile& mpu6050() {
        static const ImuDeviceProfile profile = {
            0x68,
            0x6B, 0x00,
            0x3B,
            14,
            0x1B, 0x08,
            0x1C, 0x10,
            0x1A, 0x04,
            0x75, 0x68
        };
        return profile;
    }

    static const MagDeviceProfile& hmc5883l() {
        static const MagDeviceProfile profile = {
            0x1E,
            0x00, 0x70,
            0x01, 0xA0,
            0x02, 0x00,
            0x03,
            6,
            MagSampleLayout::Hmc5883XzyBigEndian,
            0.92f
        };
        return profile;
    }

    static const MagDeviceProfile& qmc5883pCompat() {
        static const MagDeviceProfile profile = {
            0x2C,
            0x0A, 0x40,
            0x0B, 0x01,
            0x09, 0x18,
            0x01,
            6,
            MagSampleLayout::QmcXyzLittleEndian,
            0.35f
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
