#ifndef GYRO_ACCEL_DRIVER_H
#define GYRO_ACCEL_DRIVER_H

#include "../../../config.h"
#include "../../../filters/RunningMedian.h"
#include "../../../types.h"

class GyroAccelDriver {
  public:
    static constexpr float ACCEL_SCALE = 1.0f / 4096.0f;
    static constexpr float GYRO_SCALE = 1.0f / 65.5f;
    static constexpr float IIR_ALPHA = 0.15f;
    static constexpr uint8_t RAW_LEN = 14;

    void resetFilters();
    void parseRawSample(const uint8_t raw[RAW_LEN],
                        const ImuCalibration& calibration,
                        SensorBuffer& buffer,
                        uint32_t timestampUs);

  private:
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
