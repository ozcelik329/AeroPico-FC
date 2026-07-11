#include <unity.h>

#include "utils/FastMath.h"

void test_fast_math_pwm_to_unit_matches_stick_range() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, AeroPicoFastMath::pwmToUnit(1000));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, AeroPicoFastMath::pwmToUnit(1500));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, AeroPicoFastMath::pwmToUnit(2000));
}

void test_fast_math_pwm_to_unit_clamps_out_of_range() {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, AeroPicoFastMath::pwmToUnit(900));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, AeroPicoFastMath::pwmToUnit(2100));
}

void test_fast_math_pwm_to_range_uses_integer_rounding() {
    TEST_ASSERT_EQUAL_INT(1000, AeroPicoFastMath::pwmToRange(1000, 1000, 2000));
    TEST_ASSERT_EQUAL_INT(1500, AeroPicoFastMath::pwmToRange(1500, 1000, 2000));
    TEST_ASSERT_EQUAL_INT(2000, AeroPicoFastMath::pwmToRange(2000, 1000, 2000));
    TEST_ASSERT_EQUAL_INT(1250, AeroPicoFastMath::pwmToRange(1250, 1000, 2000));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fast_math_pwm_to_unit_matches_stick_range);
    RUN_TEST(test_fast_math_pwm_to_unit_clamps_out_of_range);
    RUN_TEST(test_fast_math_pwm_to_range_uses_integer_rounding);
    return UNITY_END();
}
