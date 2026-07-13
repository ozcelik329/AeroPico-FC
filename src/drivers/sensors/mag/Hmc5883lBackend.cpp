#include "Hmc5883lBackend.h"

void Hmc5883lBackend::scaleRaw(int16_t rawX,
                               int16_t rawY,
                               int16_t rawZ,
                               float& mx,
                               float& my,
                               float& mz) const {
    mx = rawX * SCALE_MILLI_GAUSS_PER_COUNT;
    my = rawY * SCALE_MILLI_GAUSS_PER_COUNT;
    mz = rawZ * SCALE_MILLI_GAUSS_PER_COUNT;
}
