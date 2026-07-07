#include <unity.h>

#include "storage/CalibrationStorage.h"

#include "../../src/storage/CalibrationStorage.cpp"

static ImuCalibration makeImuCalibration() {
    ImuCalibration imu = {};
    imu.gyroBiasX = 1.0f;
    imu.gyroBiasY = 2.0f;
    imu.gyroBiasZ = 3.0f;
    imu.accelBiasX = 0.1f;
    imu.accelBiasY = 0.2f;
    imu.accelBiasZ = 0.3f;
    imu.valid = true;
    return imu;
}

static MagCalibration makeMagCalibration() {
    MagCalibration mag = {};
    mag.hardIronX = 10.0f;
    mag.hardIronY = -5.0f;
    mag.hardIronZ = 2.5f;
    mag.valid = true;
    return mag;
}

void test_calibration_storage_creates_valid_blob() {
    CalibrationBlob blob = CalibrationStorage::makeBlob(makeImuCalibration(), makeMagCalibration());

    TEST_ASSERT_TRUE(CalibrationStorage::isValid(blob));
    TEST_ASSERT_EQUAL_UINT32(CALIBRATION_STORAGE_MAGIC, blob.magic);
}

void test_calibration_storage_rejects_corrupt_blob() {
    CalibrationBlob blob = CalibrationStorage::makeBlob(makeImuCalibration(), makeMagCalibration());
    blob.imu.gyroBiasX = 99.0f;

    TEST_ASSERT_FALSE(CalibrationStorage::isValid(blob));
}

void test_memory_calibration_storage_round_trip() {
    MemoryCalibrationStorage storage;
    CalibrationBlob saved = CalibrationStorage::makeBlob(makeImuCalibration(), makeMagCalibration());
    CalibrationBlob loaded = {};

    TEST_ASSERT_TRUE(storage.save(saved));
    TEST_ASSERT_TRUE(storage.load(loaded));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, saved.imu.gyroBiasY, loaded.imu.gyroBiasY);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, saved.mag.hardIronX, loaded.mag.hardIronX);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_calibration_storage_creates_valid_blob);
    RUN_TEST(test_calibration_storage_rejects_corrupt_blob);
    RUN_TEST(test_memory_calibration_storage_round_trip);
    return UNITY_END();
}
