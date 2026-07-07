#include <unity.h>

#include "core/PID.h"

#include "../../src/core/PID.cpp"

void test_pid_clamps_output() {
    PID pid(10.0f, 0.0f, 0.0f, -50.0f, 50.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, pid.compute(10.0f, 0.0f, 0.02f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -50.0f, pid.compute(-10.0f, 0.0f, 0.02f));
}

void test_pid_freezes_integral_when_saturating_further() {
    PID pid(0.0f, 1.0f, 0.0f, -10.0f, 10.0f, 100.0f);

    for (int i = 0; i < 30; i++) {
        pid.compute(20.0f, 0.0f, 1.0f);
    }

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pid.getIntegral());
}

void test_pid_allows_integral_to_reduce_saturation() {
    PID pid(0.0f, 1.0f, 0.0f, -10.0f, 10.0f, 100.0f);

    pid.compute(-8.0f, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -8.0f, pid.getIntegral());

    pid.compute(20.0f, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -8.0f, pid.getIntegral());

    pid.compute(5.0f, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.0f, pid.getIntegral());
}

void test_pid_handles_invalid_dt() {
    PID pid(1.0f, 0.0f, 1.0f, -100.0f, 100.0f);

    float output = pid.compute(1.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, output);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_pid_clamps_output);
    RUN_TEST(test_pid_freezes_integral_when_saturating_further);
    RUN_TEST(test_pid_allows_integral_to_reduce_saturation);
    RUN_TEST(test_pid_handles_invalid_dt);
    return UNITY_END();
}
