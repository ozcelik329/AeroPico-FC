#include <unity.h>

#include "filters/RunningMedian.h"

void test_running_median_rejects_single_spike() {
    RunningMedian<float, 5> median;

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, median.update(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, median.update(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, median.update(80.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, median.update(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, median.update(1.0f));
}

void test_running_median_sliding_window_updates() {
    RunningMedian<int, 3> median;

    TEST_ASSERT_EQUAL_INT(10, median.update(10));
    TEST_ASSERT_EQUAL_INT(10, median.update(20));
    TEST_ASSERT_EQUAL_INT(20, median.update(30));
    TEST_ASSERT_EQUAL_INT(30, median.update(40));
}

void test_running_median_reset_clears_samples() {
    RunningMedian<int, 3> median;
    median.update(10);
    median.update(20);
    median.update(30);

    TEST_ASSERT_TRUE(median.isFull());
    median.reset();

    TEST_ASSERT_EQUAL_UINT(0, median.size());
    TEST_ASSERT_EQUAL_INT(5, median.update(5));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_running_median_rejects_single_spike);
    RUN_TEST(test_running_median_sliding_window_updates);
    RUN_TEST(test_running_median_reset_clears_samples);
    return UNITY_END();
}
