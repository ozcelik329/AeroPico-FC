#ifndef GYRO_ACCEL_DRIVER_H
#define GYRO_ACCEL_DRIVER_H

#include "board/Config.h"
#include "../../../filters/RunningMedian.h"
#include "../../../types.h"
#include "Mpu6050Backend.h"

class GyroAccelDriver {
  public:
    static constexpr float ACCEL_SCALE = Mpu6050Backend::ACCEL_SCALE;
    static constexpr float GYRO_SCALE = Mpu6050Backend::GYRO_SCALE;
    static constexpr float IIR_ALPHA = 0.15f;
    static constexpr uint8_t RAW_LEN = Mpu6050Backend::RAW_LEN;

    void resetFilters();
    void parseRawSample(const uint8_t raw[RAW_LEN],
                        const ImuCalibration& calibration,
                        SensorBuffer& buffer,
                        uint32_t timestampUs);

  private:
    Mpu6050Backend _backend;
    float _axFiltered = 0.0f;
    float _ayFiltered = 0.0f;
    float _azFiltered = 0.0f;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _axMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _ayMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _azMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _gxMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _gyMedian;
    RunningMedian<float, SENSOR_MEDIAN_WINDOW> _gzMedian;
};

#endif
