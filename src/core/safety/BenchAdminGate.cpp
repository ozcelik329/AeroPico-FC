#include "BenchAdminGate.h"
#include "board/Config.h"

#if !defined(UNIT_TEST)
#include <hardware/gpio.h>
#endif

BenchAdminGate benchAdminGate;

void BenchAdminGate::init() {
#if !defined(UNIT_TEST) && BENCH_ADMIN_FORCE_ARM_ENABLED
    gpio_init(PIN_BENCH_ADMIN_GND);
    gpio_set_dir(PIN_BENCH_ADMIN_GND, GPIO_OUT);
    gpio_put(PIN_BENCH_ADMIN_GND, 0);

    gpio_init(PIN_BENCH_ADMIN_SENSE);
    gpio_set_dir(PIN_BENCH_ADMIN_SENSE, GPIO_IN);
    gpio_pull_up(PIN_BENCH_ADMIN_SENSE);
#endif
}

bool BenchAdminGate::forceArmActive() const {
#if !defined(UNIT_TEST) && BENCH_ADMIN_FORCE_ARM_ENABLED
    return gpio_get(PIN_BENCH_ADMIN_SENSE) == 0;
#else
    return false;
#endif
}
