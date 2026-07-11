#include <unity.h>

#include "drivers/sensors/SensorCalibration.h"

#include "../../src/drivers/sensors/SensorCalibration.cpp"

static MpuRawCalibrationSample makeSample(int16_t gx, int16_t gy, int16_t gz, int16_t temp) {
    MpuRawCalibrationSample sample = {};
    sample.ax = 0;
    sample.ay = 0;
    sample.az = 4096;
    sample.gx = gx;
    sample.gy = gy;
    sample.gz = gz;
    sample.temp = temp;
    return sample;
}

void test_sensor_calibration_rejects_too_few_samples() {
    SensorCalibration calibration;
    calibration.reset();
    calibration.observeMpuRaw(makeSample(65, 0, 0, 0));

    SensorCalibrationResult result = calibration.finish(2);
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_FALSE(result.imu.valid);
}

void test_sensor_calibration_computes_imu_biases() {
    SensorCalibration calibration;
    calibration.reset();
    calibration.observeMpuRaw(makeSample(65, -131, 0, 0));
    calibration.observeMpuRaw(makeSample(65, -131, 0, 0));

    SensorCalibrationResult result = calibration.finish(2);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_TRUE(result.imu.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.992f, result.imu.gyroBiasX);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -2.0f, result.imu.gyroBiasY);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result.imu.accelBiasX);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result.imu.accelBiasY);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result.imu.accelBiasZ);
}

void test_sensor_calibration_defaults_temp_coeff_without_temp_span() {
    SensorCalibration calibration;
    calibration.reset();
    calibration.observeMpuRaw(makeSample(65, 0, 0, 0));
    calibration.observeMpuRaw(makeSample(65, 0, 0, 0));

    SensorCalibrationResult result = calibration.finish(2);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.004f, result.imu.gyroTempCoeff);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sensor_calibration_rejects_too_few_samples);
    RUN_TEST(test_sensor_calibration_computes_imu_biases);
    RUN_TEST(test_sensor_calibration_defaults_temp_coeff_without_temp_span);
    return UNITY_END();
}
