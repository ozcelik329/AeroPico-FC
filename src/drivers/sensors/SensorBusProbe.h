#ifndef SENSOR_BUS_PROBE_H
#define SENSOR_BUS_PROBE_H

#include "../../types.h"
#include "../../hal/HAL_I2C.h"
#include "SensorBackendRegistry.h"

class SensorBusProbe {
  public:
    static SensorBusProbeSnapshot scanMain(IHALI2C& bus) {
        SensorBusProbeSnapshot snapshot = {};
        scanInto(bus, snapshot.mainDevices, snapshot.mainCount, snapshot.mainOverflow);
        classify(snapshot);
        return snapshot;
    }

    static void scanAux(IHALI2C& bus, SensorBusProbeSnapshot& snapshot) {
        snapshot.bypassEnabled = true;
        scanInto(bus, snapshot.auxDevices, snapshot.auxCount, snapshot.auxOverflow);
        classify(snapshot);
    }

  private:
    static bool probeAddress(IHALI2C& bus, uint8_t address) {
        return bus.probeAddress(address);
    }

    static void scanInto(IHALI2C& bus,
                         SensorBusDevice* devices,
                         uint8_t& count,
                         bool& overflow) {
        count = 0;
        overflow = false;
        for (uint8_t address = 0x08; address <= 0x77; ++address) {
            if (!probeAddress(bus, address)) {
                continue;
            }
            if (count < SensorBusProbeSnapshot::MAX_DEVICES) {
                devices[count++] = {address, SensorBusDeviceRole::Unknown};
            } else {
                overflow = true;
            }
        }
    }

    static void classifyList(SensorBusDevice* devices, uint8_t count) {
        const uint8_t imuAddr = SensorBackendRegistry::mpu6050().address;
        const uint8_t baroAddr = SensorBackendRegistry::bmp085().address;
        const uint8_t hmcAddr = SensorBackendRegistry::hmc5883l().address;

        for (uint8_t i = 0; i < count; ++i) {
            switch (devices[i].address) {
                case 0x2C:
                    devices[i].role = SensorBusDeviceRole::UnsupportedMag;
                    break;
                default:
                    if (devices[i].address == imuAddr) {
                        devices[i].role = SensorBusDeviceRole::Imu;
                    } else if (devices[i].address == baroAddr) {
                        devices[i].role = SensorBusDeviceRole::Baro;
                    } else if (devices[i].address == hmcAddr) {
                        devices[i].role = SensorBusDeviceRole::Mag;
                    }
                    break;
            }
        }
    }

    static void classify(SensorBusProbeSnapshot& snapshot) {
        classifyList(snapshot.mainDevices, snapshot.mainCount);
        classifyList(snapshot.auxDevices, snapshot.auxCount);
    }
};

#endif
