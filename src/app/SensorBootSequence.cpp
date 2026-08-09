#include "SensorBootSequence.h"
#include "board/Config.h"
#include "../drivers/Sensors.h"
#include "../storage/CalibrationStorage.h"
#include "../utils/BootLogger.h"

bool SensorBootSequence::run(SensorManager& sensors, RPFlashCalibrationStorage& calibrationStorage) {
    SensorCapabilityStatus sensorCapabilities = sensors.capabilities();
    const bool imuOk = sensorCapabilities.imuAvailable;
    if (imuOk) {
        char whoamiText[16];
        snprintf(whoamiText, sizeof(whoamiText), "WHOAMI=0x%02X", sensors.getLastWhoAmI());
        BootLogger::okWithValue("MPU6050", whoamiText);

        CalibrationBlob calibrationBlob = {};
        if (calibrationStorage.load(calibrationBlob)) {
            sensors.setImuCalibration(calibrationBlob.imu);
            sensors.setMagCalibration(calibrationBlob.mag);
            BootLogger::ok("Calibration Load");
        } else if (sensors.runBootCalibration()) {
            BootLogger::ok("Gyro/Accel Bias Cal");
            CalibrationBlob savedCalibration = CalibrationStorage::makeBlob(sensors.getImuCalibration(),
                                                                            sensors.getMagCalibration());
            if (calibrationStorage.save(savedCalibration)) {
                BootLogger::ok("Calibration Save");
            } else {
                BootLogger::warn("Calibration Save", "Flash kaydi basarisiz");
            }
        } else {
            BootLogger::warn("Gyro/Accel Bias Cal", "Yetersiz ornek veya basarisiz");
        }
    } else {
        BootLogger::fail("MPU6050", "WHOAMI dogrulanamadi veya bagli degil");
        BootLogger::warn("Sensor Fault", sensors.getFaultText());
    }

#ifdef USE_GY87
    sensorCapabilities = sensors.capabilities();
    if (sensorCapabilities.baroAvailable) BootLogger::ok("BMP085");
    else BootLogger::fail("BMP085", "Barometre bulunamadi");

    if (sensorCapabilities.magAvailable) BootLogger::ok("HMC5883L");
    else BootLogger::fail("HMC5883L", "Manyetometre bulunamadi");
#endif

    return imuOk;
}
