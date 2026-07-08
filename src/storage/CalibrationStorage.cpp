#include "CalibrationStorage.h"

#if !defined(UNIT_TEST)
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/platform.h"
#endif

static_assert(sizeof(CalibrationBlob) <= 256, "CalibrationBlob must fit in one flash page");

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

bool RPFlashCalibrationStorage::load(CalibrationBlob& blob) {
#if defined(UNIT_TEST)
    (void)blob;
    return false;
#else
    constexpr uint32_t FLASH_TARGET_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
    const auto* stored = reinterpret_cast<const CalibrationBlob*>(XIP_BASE + FLASH_TARGET_OFFSET);
    blob = *stored;
    return CalibrationStorage::isValid(blob);
#endif
}

bool RPFlashCalibrationStorage::save(const CalibrationBlob& blob) {
    if (!CalibrationStorage::isValid(blob)) {
        return false;
    }

#if defined(UNIT_TEST)
    (void)blob;
    return false;
#else
    constexpr uint32_t FLASH_TARGET_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
    constexpr size_t PAGE_SIZE = FLASH_PAGE_SIZE;
    alignas(4) uint8_t page[PAGE_SIZE];

    CalibrationBlob existing = {};
    if (load(existing) && memcmp(&existing, &blob, sizeof(CalibrationBlob)) == 0) {
        return true;
    }

    memset(page, 0xFF, sizeof(page));
    memcpy(page, &blob, sizeof(CalibrationBlob));

    uint32_t irqState = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, page, PAGE_SIZE);
    restore_interrupts(irqState);
    return true;
#endif
}
