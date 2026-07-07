#include "CalibrationStorage.h"

CalibrationBlob CalibrationStorage::makeBlob(const ImuCalibration& imu, const MagCalibration& mag) {
    CalibrationBlob blob = {};
    blob.magic = CALIBRATION_STORAGE_MAGIC;
    blob.version = CALIBRATION_STORAGE_VERSION;
    blob.size = sizeof(CalibrationBlob);
    blob.imu = imu;
    blob.mag = mag;
    blob.checksum = checksum(blob);
    return blob;
}

uint32_t CalibrationStorage::checksum(const CalibrationBlob& blob) {
    CalibrationBlob copy = blob;
    copy.checksum = 0;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&copy);
    uint32_t sum = 2166136261u;
    for (size_t i = 0; i < sizeof(CalibrationBlob); i++) {
        sum ^= bytes[i];
        sum *= 16777619u;
    }
    return sum;
}

bool CalibrationStorage::isValid(const CalibrationBlob& blob) {
    if (blob.magic != CALIBRATION_STORAGE_MAGIC) return false;
    if (blob.version != CALIBRATION_STORAGE_VERSION) return false;
    if (blob.size != sizeof(CalibrationBlob)) return false;
    if (blob.checksum != checksum(blob)) return false;
    if (!blob.imu.valid && !blob.mag.valid) return false;
    return true;
}

bool MemoryCalibrationStorage::load(CalibrationBlob& blob) {
    if (!_hasBlob || !CalibrationStorage::isValid(_blob)) return false;
    blob = _blob;
    return true;
}

bool MemoryCalibrationStorage::save(const CalibrationBlob& blob) {
    if (!CalibrationStorage::isValid(blob)) return false;
    _blob = blob;
    _hasBlob = true;
    return true;
}

void MemoryCalibrationStorage::clear() {
    _blob = {};
    _hasBlob = false;
}
