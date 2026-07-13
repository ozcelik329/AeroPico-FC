#include "RP2350_GPIO.h"

#if !defined(UNIT_TEST)
#include <hardware/gpio.h>
#endif

void RP2350GPIO::configure(uint8_t pin, HALGpioMode mode) {
#if !defined(UNIT_TEST)
    gpio_init(pin);
    switch (mode) {
        case HALGpioMode::Output:
            gpio_set_dir(pin, GPIO_OUT);
            break;
        case HALGpioMode::InputPullup:
            gpio_set_dir(pin, GPIO_IN);
            gpio_pull_up(pin);
            break;
        case HALGpioMode::InputPulldown:
            gpio_set_dir(pin, GPIO_IN);
            gpio_pull_down(pin);
            break;
        case HALGpioMode::Input:
        default:
            gpio_set_dir(pin, GPIO_IN);
            break;
    }
#else
    (void)pin;
    (void)mode;
#endif
}

void RP2350GPIO::write(uint8_t pin, bool high) {
#if !defined(UNIT_TEST)
    gpio_put(pin, high ? 1 : 0);
#else
    (void)pin;
    (void)high;
#endif
}

bool RP2350GPIO::read(uint8_t pin) const {
#if !defined(UNIT_TEST)
    return gpio_get(pin) != 0;
#else
    (void)pin;
    return false;
#endif
}
