#include "Hmc5883lBackend.h"

void Hmc5883lBackend::scaleRaw(int16_t rawX,
                               int16_t rawY,
                               int16_t rawZ,
                               float& mx,
                               float& my,
                               float& mz) const {
    scaleRaw(rawX, rawY, rawZ, SCALE_MILLI_GAUSS_PER_COUNT, mx, my, mz);
}

void Hmc5883lBackend::scaleRaw(int16_t rawX,
                               int16_t rawY,
                               int16_t rawZ,
                               float scaleMilliGaussPerCount,
                               float& mx,
                               float& my,
                               float& mz) const {
    mx = rawX * scaleMilliGaussPerCount;
    my = rawY * scaleMilliGaussPerCount;
    mz = rawZ * scaleMilliGaussPerCount;
}
