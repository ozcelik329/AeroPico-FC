#include <unity.h>

#include "core/ThreadSafeRingBuffer.h"

struct Sample {
    int value;
};

void test_ringbuffer_push_pop_single_consumer() {
    ThreadSafeRingBuffer<Sample, 4> buf;
    Sample in{42};
    TEST_ASSERT_TRUE(buf.push(in));

    Sample out{0};
    TEST_ASSERT_TRUE(buf.pop(out));
    TEST_ASSERT_EQUAL_INT(42, out.value);
    TEST_ASSERT_TRUE(buf.isEmpty());
}

void test_ringbuffer_peek_does_not_consume() {
    ThreadSafeRingBuffer<Sample, 4> buf;
    Sample in{7};
    TEST_ASSERT_TRUE(buf.push(in));

    Sample peeked{0};
    TEST_ASSERT_TRUE(buf.peek(peeked));
    TEST_ASSERT_EQUAL_INT(7, peeked.value);

    Sample out{0};
    TEST_ASSERT_TRUE(buf.pop(out));
    TEST_ASSERT_EQUAL_INT(7, out.value);
    TEST_ASSERT_TRUE(buf.isEmpty());
}

void test_ringbuffer_full_rejects_push() {
    ThreadSafeRingBuffer<Sample, 4> buf;
    TEST_ASSERT_TRUE(buf.push({1}));
    TEST_ASSERT_TRUE(buf.push({2}));
    TEST_ASSERT_TRUE(buf.push({3}));
    TEST_ASSERT_FALSE(buf.push({4}));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ringbuffer_push_pop_single_consumer);
    RUN_TEST(test_ringbuffer_peek_does_not_consume);
    RUN_TEST(test_ringbuffer_full_rejects_push);
    return UNITY_END();
}
