#include "CalibrationStorage.h"

#if !defined(UNIT_TEST)
#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico/platform.h"

namespace {
// Calibration uses sectors -6 and -5. Parameter storage owns -4 and -3,
// while the Arduino core reserves the final sector for EEPROM emulation.
constexpr uint32_t CALIBRATION_FLASH_BASE_OFFSET =
    PICO_FLASH_SIZE_BYTES - (6 * FLASH_SECTOR_SIZE);
constexpr uint32_t PREVIOUS_CALIBRATION_FLASH_BASE_OFFSET =
    PICO_FLASH_SIZE_BYTES - (CALIBRATION_STORAGE_SLOT_COUNT * FLASH_SECTOR_SIZE);

struct CalibrationFlashWriteContext {
    uint32_t offset;
    const uint8_t* page;
};

struct LegacyCalibrationBlob {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    ImuCalibration imu;
    MagCalibration mag;
    uint32_t checksum;
};

uint32_t legacyCalibrationChecksum(const LegacyCalibrationBlob& blob) {
    LegacyCalibrationBlob copy = blob;
    copy.checksum = 0;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&copy);
    uint32_t sum = 2166136261u;
    for (size_t i = 0; i < sizeof(LegacyCalibrationBlob); ++i) {
        sum ^= bytes[i];
        sum *= 16777619u;
    }
    return sum;
}

bool legacyCalibrationValid(const LegacyCalibrationBlob& blob) {
    return blob.magic == CALIBRATION_STORAGE_MAGIC &&
           blob.version == CALIBRATION_STORAGE_VERSION &&
           blob.size == sizeof(LegacyCalibrationBlob) &&
           blob.checksum == legacyCalibrationChecksum(blob) &&
           (blob.imu.valid || blob.mag.valid);
}

void loadCalibrationJournal(uint32_t baseOffset, CalibrationBlob& best, bool& found) {
    for (uint8_t slot = 0; slot < CALIBRATION_STORAGE_SLOT_COUNT; ++slot) {
        const uint32_t offset = baseOffset + ((uint32_t)slot * FLASH_SECTOR_SIZE);
        const auto* stored = reinterpret_cast<const CalibrationBlob*>(XIP_BASE + offset);
        const CalibrationBlob candidate = *stored;
        if (CalibrationStorage::isValid(candidate) &&
            (!found || candidate.generation > best.generation)) {
            best = candidate;
            found = true;
        }
    }
}

bool calibrationSlotMatches(uint32_t offset, const CalibrationBlob& expected) {
    const auto* stored = reinterpret_cast<const CalibrationBlob*>(XIP_BASE + offset);
    const CalibrationBlob candidate = *stored;
    return CalibrationStorage::isValid(candidate) &&
           candidate.generation == expected.generation &&
           memcmp(&candidate, &expected, sizeof(CalibrationBlob)) == 0;
}

void __not_in_flash_func(programCalibrationFlash)(void* opaque) {
    auto* context = static_cast<CalibrationFlashWriteContext*>(opaque);
    flash_range_erase(context->offset, FLASH_SECTOR_SIZE);
    flash_range_program(context->offset, context->page, FLASH_PAGE_SIZE);
}
}
#endif

static_assert(sizeof(CalibrationBlob) <= 256, "CalibrationBlob must fit in one flash page");

CalibrationBlob CalibrationStorage::makeBlob(const ImuCalibration& imu, const MagCalibration& mag) {
    CalibrationBlob blob = {};
    blob.magic = CALIBRATION_STORAGE_MAGIC;
    blob.version = CALIBRATION_STORAGE_VERSION;
    blob.size = sizeof(CalibrationBlob);
    blob.generation = 1;
    blob.imu = imu;
    blob.mag = mag;
    blob.checksum = checksum(blob);
    return blob;
}

CalibrationBlob CalibrationStorage::withGeneration(CalibrationBlob blob, uint32_t generation) {
    blob.generation = generation == 0 ? 1 : generation;
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
    bool found = false;
    CalibrationBlob best = {};
    loadCalibrationJournal(CALIBRATION_FLASH_BASE_OFFSET, best, found);
    loadCalibrationJournal(PREVIOUS_CALIBRATION_FLASH_BASE_OFFSET, best, found);
    if (!found) {
        constexpr uint32_t LEGACY_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
        const auto* legacy = reinterpret_cast<const LegacyCalibrationBlob*>(XIP_BASE + LEGACY_OFFSET);
        LegacyCalibrationBlob legacyBlob = *legacy;
        if (!legacyCalibrationValid(legacyBlob)) {
            return false;
        }
        blob = CalibrationStorage::makeBlob(legacyBlob.imu, legacyBlob.mag);
        return true;
    }
    blob = best;
    return true;
#endif
}

bool RPFlashCalibrationStorage::save(const CalibrationBlob& blob) {
    _lastError = Error::None;
    if (!CalibrationStorage::isValid(blob)) {
        _lastError = Error::InvalidBlob;
        return false;
    }

#if defined(UNIT_TEST)
    (void)blob;
    return false;
#else
    constexpr size_t PAGE_SIZE = FLASH_PAGE_SIZE;
    alignas(FLASH_PAGE_SIZE) uint8_t page[PAGE_SIZE];

    CalibrationBlob existing = {};
    const bool hasExisting = load(existing);
    if (hasExisting && memcmp(&existing.imu, &blob.imu, sizeof(ImuCalibration)) == 0 &&
        memcmp(&existing.mag, &blob.mag, sizeof(MagCalibration)) == 0) {
        return true;
    }
    const uint32_t nextGeneration = hasExisting ? existing.generation + 1U : 1U;
    const uint8_t slot = (uint8_t)(nextGeneration % CALIBRATION_STORAGE_SLOT_COUNT);
    const CalibrationBlob journalBlob = CalibrationStorage::withGeneration(blob, nextGeneration);

    memset(page, 0xFF, sizeof(page));
    memcpy(page, &journalBlob, sizeof(CalibrationBlob));

    CalibrationFlashWriteContext context = {
        CALIBRATION_FLASH_BASE_OFFSET + ((uint32_t)slot * FLASH_SECTOR_SIZE),
        page
    };
    constexpr uint32_t FLASH_SAFE_TIMEOUT_MS = 2500;
    constexpr uint8_t FLASH_WRITE_ATTEMPTS = 3;
    for (uint8_t attempt = 0; attempt < FLASH_WRITE_ATTEMPTS; ++attempt) {
        const int result = flash_safe_execute(
            programCalibrationFlash,
            &context,
            FLASH_SAFE_TIMEOUT_MS
        );

        // The callback may have completed even when the SDK reports an exit
        // timeout, so verified flash contents are the authoritative result.
        if (calibrationSlotMatches(context.offset, journalBlob)) {
            _lastError = Error::None;
            return true;
        }

        switch (result) {
            case PICO_ERROR_TIMEOUT:
                _lastError = Error::FlashTimeout;
                break;
            case PICO_ERROR_NOT_PERMITTED:
                _lastError = Error::FlashNotPermitted;
                break;
            case PICO_ERROR_INSUFFICIENT_RESOURCES:
                _lastError = Error::FlashResources;
                break;
            case PICO_OK:
                _lastError = Error::VerifyFailed;
                break;
            default:
                _lastError = Error::FlashError;
                break;
        }
        delay(5);
    }
    return false;
#endif
}

const char* RPFlashCalibrationStorage::lastError() const {
    switch (_lastError) {
        case Error::None: return "NONE";
        case Error::InvalidBlob: return "INVALID_BLOB";
        case Error::FlashTimeout: return "FLASH_TIMEOUT";
        case Error::FlashNotPermitted: return "FLASH_NOT_PERMITTED";
        case Error::FlashResources: return "FLASH_RESOURCES";
        case Error::VerifyFailed: return "VERIFY_FAILED";
        case Error::FlashError:
        default: return "FLASH_ERROR";
    }
}
