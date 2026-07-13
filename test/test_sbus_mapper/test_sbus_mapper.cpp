#include <unity.h>

#include "drivers/rc/SbusMapper.h"

void test_sbus_mapper_maps_nominal_range_without_float() {
    TEST_ASSERT_EQUAL_UINT16(1000, SbusMapper::rawToPwm(172));
    TEST_ASSERT_EQUAL_UINT16(1500, SbusMapper::rawToPwm(992));
    TEST_ASSERT_EQUAL_UINT16(2000, SbusMapper::rawToPwm(1811));
}

void test_sbus_mapper_clamps_out_of_range_values() {
    TEST_ASSERT_EQUAL_UINT16(PWM_MIN, SbusMapper::rawToPwm(0));
    TEST_ASSERT_EQUAL_UINT16(PWM_MAX, SbusMapper::rawToPwm(2500));
}

void test_sbus_mapper_applies_complete_frame() {
    SbusFrameView frame = {};
    for (size_t i = 0; i < 16; ++i) {
        frame.channels[i] = 172 + (uint16_t)i;
    }
    uint16_t out[16] = {};

    SbusMapper::applyFrame(frame, out, 16);

    TEST_ASSERT_EQUAL_UINT16(1000, out[0]);
    TEST_ASSERT_TRUE(out[15] >= 1000);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sbus_mapper_maps_nominal_range_without_float);
    RUN_TEST(test_sbus_mapper_clamps_out_of_range_values);
    RUN_TEST(test_sbus_mapper_applies_complete_frame);
    return UNITY_END();
}
