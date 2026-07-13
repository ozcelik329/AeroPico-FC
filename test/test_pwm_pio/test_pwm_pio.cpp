#include <unity.h>

#define PICO_NO_HARDWARE 1
#include "../../src/drivers/pwm.pio.h"

void test_servo_pio_drives_pin_high_and_low() {
    bool hasSetHigh = false;
    bool hasSetLow = false;
    bool hasPullBlock = false;

    for (uint8_t i = 0; i < sizeof(pwm_servo_program_instructions) / sizeof(pwm_servo_program_instructions[0]); ++i) {
        uint16_t instruction = pwm_servo_program_instructions[i];
        hasSetHigh = hasSetHigh || instruction == 0xe001;
        hasSetLow = hasSetLow || instruction == 0xe000;
        hasPullBlock = hasPullBlock || instruction == 0x80a0;
    }

    TEST_ASSERT_TRUE(hasPullBlock);
    TEST_ASSERT_TRUE(hasSetHigh);
    TEST_ASSERT_TRUE(hasSetLow);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(7, sizeof(pwm_servo_program_instructions) / sizeof(pwm_servo_program_instructions[0]));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_servo_pio_drives_pin_high_and_low);
    return UNITY_END();
}
