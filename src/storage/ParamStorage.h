#ifndef PARAM_STORAGE_H
#define PARAM_STORAGE_H

#include <Arduino.h>

static constexpr uint32_t PARAM_STORAGE_MAGIC = 0x41505052; // APPR
static constexpr uint16_t PARAM_STORAGE_VERSION = 1;
static constexpr size_t PARAM_STORAGE_MAX_VALUES = 48;
static constexpr uint8_t PARAM_STORAGE_SLOT_COUNT = 2;

struct ParamStorageBlob {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    uint32_t generation;
    float values[PARAM_STORAGE_MAX_VALUES];
    uint32_t checksum;
};

class IParamStorage {
  public:
    virtual ~IParamStorage() {}
    virtual bool load(ParamStorageBlob& blob) = 0;
    virtual bool save(const ParamStorageBlob& blob) = 0;
};

class ParamStorage {
  public:
    static ParamStorageBlob makeBlob(const float* values, size_t count);
    static ParamStorageBlob withGeneration(ParamStorageBlob blob, uint32_t generation);
    static bool hasValidEnvelope(const ParamStorageBlob& blob);
    static bool isValid(const ParamStorageBlob& blob, size_t expectedCount);
    static uint32_t checksum(const ParamStorageBlob& blob);
};

class MemoryParamStorage : public IParamStorage {
  public:
    bool load(ParamStorageBlob& blob) override;
    bool save(const ParamStorageBlob& blob) override;
    void clear();

  private:
    ParamStorageBlob _blob = {};
    bool _hasBlob = false;
};

class RPFlashParamStorage : public IParamStorage {
  public:
    bool load(ParamStorageBlob& blob) override;
    bool save(const ParamStorageBlob& blob) override;
};

#endif
