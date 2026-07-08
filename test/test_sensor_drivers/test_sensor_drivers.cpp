#include <unity.h>

#include "drivers/sensors/baro/BaroDriver.h"
#include "drivers/sensors/gyro/GyroAccelDriver.h"
#include "drivers/sensors/mag/MagDriver.h"

#include "../../src/drivers/sensors/baro/BaroDriver.cpp"
#include "../../src/drivers/sensors/gyro/GyroAccelDriver.cpp"
#include "../../src/drivers/sensors/mag/MagDriver.cpp"

static void writeInt16(uint8_t* raw, int index, int16_t value) {
    raw[index] = (uint8_t)((value >> 8) & 0xFF);
    raw[index + 1] = (uint8_t)(value & 0xFF);
}

void test_gyro_accel_driver_parses_raw_sample() {
    GyroAccelDriver driver;
    uint8_t raw[GyroAccelDriver::RAW_LEN] = {};
    writeInt16(raw, 0, 4096);
    writeInt16(raw, 2, 0);
    writeInt16(raw, 4, 4096);
    writeInt16(raw, 6, 0);
    writeInt16(raw, 8, 65);
    writeInt16(raw, 10, 0);
    writeInt16(raw, 12, -65);

    ImuCalibration calibration = {};
    SensorBuffer buffer = {};
    driver.parseRawSample(raw, calibration, buffer, 1234);

    TEST_ASSERT_TRUE(buffer.valid);
    TEST_ASSERT_EQUAL((int)SensorHealth::Ok, (int)buffer.health);
    TEST_ASSERT_EQUAL_UINT32(1234, buffer.timestamp);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 36.53f, buffer.tempC);
}

void test_mag_driver_applies_hard_iron_calibration() {
    MagDriver driver;
    MagCalibration calibration;
    calibration.hardIronX = 10.0f;
    calibration.hardIronY = -5.0f;
    calibration.hardIronZ = 2.0f;
    calibration.valid = true;
    driver.setCalibration(calibration);

    SensorBuffer buffer = {};
    driver.applySample(100, -50, 25, buffer);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 82.0f, buffer.mx);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -41.0f, buffer.my);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 21.0f, buffer.mz);
}

void test_mag_driver_collects_hard_iron_calibration() {
    MagDriver driver;
    driver.beginCalibration();
    TEST_ASSERT_TRUE(driver.observeCalibrationSample(-10.0f, -4.0f, 1.0f));
    TEST_ASSERT_TRUE(driver.observeCalibrationSample(30.0f, 6.0f, 5.0f));

    MagCalibration calibration = driver.finishCalibration();

    TEST_ASSERT_TRUE(calibration.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, calibration.hardIronX);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, calibration.hardIronY);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, calibration.hardIronZ);
}

void test_baro_driver_rejects_pressure_without_calibration() {
    BaroDriver driver;
    SensorBuffer buffer = {};

    TEST_ASSERT_FALSE(driver.applyRawPressure(23843, buffer));
}

void test_baro_driver_computes_pressure_with_calibration() {
    BaroDriver driver;
    uint8_t calibration[22] = {
        0x01, 0x98, 0xFF, 0xB8, 0xC7, 0xD1, 0x80, 0x00,
        0x80, 0x00, 0x5A, 0x71, 0x18, 0x2E, 0x00, 0x04,
        0x80, 0x00, 0xDD, 0xF9, 0x0B, 0x34
    };

    TEST_ASSERT_TRUE(driver.loadCalibration(calibration));
    driver.setRawTemperature(27898);

    SensorBuffer buffer = {};
    TEST_ASSERT_TRUE(driver.applyRawPressure(23843, buffer));
    TEST_ASSERT_TRUE(buffer.pressure > 0.0f);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_gyro_accel_driver_parses_raw_sample);
    RUN_TEST(test_mag_driver_applies_hard_iron_calibration);
    RUN_TEST(test_mag_driver_collects_hard_iron_calibration);
    RUN_TEST(test_baro_driver_rejects_pressure_without_calibration);
    RUN_TEST(test_baro_driver_computes_pressure_with_calibration);
    return UNITY_END();
}
