#ifndef FAST_MATH_H
#define FAST_MATH_H

#include <stdint.h>

#if defined(__GNUC__)
#define AEROPICO_ALWAYS_INLINE inline __attribute__((always_inline))
#define AEROPICO_LIKELY(x) __builtin_expect(!!(x), 1)
#define AEROPICO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define AEROPICO_ALWAYS_INLINE inline
#define AEROPICO_LIKELY(x) (x)
#define AEROPICO_UNLIKELY(x) (x)
#endif

namespace AeroPicoFastMath {

static constexpr uint16_t PWM_US_MIN = 1000;
static constexpr uint16_t PWM_US_MID = 1500;
static constexpr uint16_t PWM_US_MAX = 2000;

AEROPICO_ALWAYS_INLINE uint16_t clampPwmUs(uint16_t value) {
    if (AEROPICO_LIKELY(value >= PWM_US_MIN && value <= PWM_US_MAX)) {
        return value;
    }
    return value < PWM_US_MIN ? PWM_US_MIN : PWM_US_MAX;
}

AEROPICO_ALWAYS_INLINE float pwmToUnit(uint16_t value) {
    const int32_t centered = (int32_t)clampPwmUs(value) - (int32_t)PWM_US_MID;
    return (float)centered * 0.002f;
}

AEROPICO_ALWAYS_INLINE int pwmToRange(uint16_t value, int outMin, int outMax) {
    const int32_t clamped = (int32_t)clampPwmUs(value) - (int32_t)PWM_US_MIN;
    return outMin + (int)((clamped * (outMax - outMin) + 500) / 1000);
}

}  // namespace AeroPicoFastMath

#endif
