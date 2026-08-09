#ifndef SENSOR_BOOT_SEQUENCE_H
#define SENSOR_BOOT_SEQUENCE_H

class SensorManager;
class RPFlashCalibrationStorage;

namespace SensorBootSequence {
bool run(SensorManager& sensors, RPFlashCalibrationStorage& calibrationStorage);
}

#endif
