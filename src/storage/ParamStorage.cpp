#include "ParamStorage.h"

#include <string.h>

#if !defined(UNIT_TEST)
#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico/platform.h"

namespace {
struct ParamFlashWriteContext {
    uint32_t offset;
    const uint8_t* page;
};

void __not_in_flash_func(programParamFlash)(void* opaque) {
    auto* context = static_cast<ParamFlashWriteContext*>(opaque);
    flash_range_erase(context->offset, FLASH_SECTOR_SIZE);
    flash_range_program(context->offset, context->page, FLASH_PAGE_SIZE);
}
}
#endif

static_assert(sizeof(ParamStorageBlob) <= 256, "ParamStorageBlob must fit in one flash page");

ParamStorageBlob ParamStorage::makeBlob(const float* values, size_t count) {
    ParamStorageBlob blob = {};
    blob.magic = PARAM_STORAGE_MAGIC;
    blob.version = PARAM_STORAGE_VERSION;
    blob.count = count > PARAM_STORAGE_MAX_VALUES ? PARAM_STORAGE_MAX_VALUES : (uint16_t)count;

    for (size_t i = 0; i < blob.count; i++) {
        blob.values[i] = values[i];
    }

    blob.checksum = checksum(blob);
    return blob;
}

uint32_t ParamStorage::checksum(const ParamStorageBlob& blob) {
    ParamStorageBlob copy = blob;
    copy.checksum = 0;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&copy);
    uint32_t sum = 2166136261u;
    for (size_t i = 0; i < sizeof(ParamStorageBlob); i++) {
        sum ^= bytes[i];
        sum *= 16777619u;
    }
    return sum;
}

bool ParamStorage::isValid(const ParamStorageBlob& blob, size_t expectedCount) {
    if (blob.magic != PARAM_STORAGE_MAGIC) return false;
    if (blob.version != PARAM_STORAGE_VERSION) return false;
    if (blob.count != expectedCount) return false;
    if (blob.count > PARAM_STORAGE_MAX_VALUES) return false;
    if (blob.checksum != checksum(blob)) return false;
    return true;
}

bool MemoryParamStorage::load(ParamStorageBlob& blob) {
    if (!_hasBlob) return false;
    blob = _blob;
    return true;
}

bool MemoryParamStorage::save(const ParamStorageBlob& blob) {
    _blob = blob;
    _hasBlob = true;
    return true;
}

void MemoryParamStorage::clear() {
    _blob = {};
    _hasBlob = false;
}

bool RPFlashParamStorage::load(ParamStorageBlob& blob) {
#if defined(UNIT_TEST)
    (void)blob;
    return false;
#else
    constexpr uint32_t FLASH_TARGET_OFFSET = PICO_FLASH_SIZE_BYTES - (2 * FLASH_SECTOR_SIZE);
    const auto* stored = reinterpret_cast<const ParamStorageBlob*>(XIP_BASE + FLASH_TARGET_OFFSET);
    blob = *stored;
    return ParamStorage::isValid(blob, blob.count);
#endif
}

bool RPFlashParamStorage::save(const ParamStorageBlob& blob) {
    if (!ParamStorage::isValid(blob, blob.count)) {
        return false;
    }

#if defined(UNIT_TEST)
    (void)blob;
    return false;
#else
    constexpr uint32_t FLASH_TARGET_OFFSET = PICO_FLASH_SIZE_BYTES - (2 * FLASH_SECTOR_SIZE);
    constexpr size_t PAGE_SIZE = FLASH_PAGE_SIZE;
    alignas(4) uint8_t page[PAGE_SIZE];

    ParamStorageBlob existing = {};
    if (load(existing) && memcmp(&existing, &blob, sizeof(ParamStorageBlob)) == 0) {
        return true;
    }

    memset(page, 0xFF, sizeof(page));
    memcpy(page, &blob, sizeof(ParamStorageBlob));

    ParamFlashWriteContext context = {FLASH_TARGET_OFFSET, page};
    constexpr uint32_t FLASH_SAFE_TIMEOUT_MS = 1000;
    return flash_safe_execute(programParamFlash, &context, FLASH_SAFE_TIMEOUT_MS) == PICO_OK;
#endif
}
