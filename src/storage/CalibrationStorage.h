#ifndef CALIBRATION_STORAGE_H
#define CALIBRATION_STORAGE_H

#include <Arduino.h>
#include "../types.h"

static constexpr uint32_t CALIBRATION_STORAGE_MAGIC = 0x41504341; // APCA
static constexpr uint16_t CALIBRATION_STORAGE_VERSION = 1;

struct CalibrationBlob {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    ImuCalibration imu;
    MagCalibration mag;
    uint32_t checksum;
};

class ICalibrationStorage {
  public:
    virtual ~ICalibrationStorage() {}
    virtual bool load(CalibrationBlob& blob) = 0;
    virtual bool save(const CalibrationBlob& blob) = 0;
};

class CalibrationStorage {
  public:
    static CalibrationBlob makeBlob(const ImuCalibration& imu, const MagCalibration& mag);
    static bool isValid(const CalibrationBlob& blob);
    static uint32_t checksum(const CalibrationBlob& blob);
};

class MemoryCalibrationStorage : public ICalibrationStorage {
  public:
    bool load(CalibrationBlob& blob) override;
    bool save(const CalibrationBlob& blob) override;
    void clear();

  private:
    CalibrationBlob _blob = {};
    bool _hasBlob = false;
};

class RPFlashCalibrationStorage : public ICalibrationStorage {
  public:
    bool load(CalibrationBlob& blob) override;
    bool save(const CalibrationBlob& blob) override;
};

#endif
