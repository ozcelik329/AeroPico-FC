#include <unity.h>

#include "core/data/SystemBlackboard.h"

#include "../../src/core/data/SystemBlackboard.cpp"

void setUp() {}
void tearDown() {}

void test_topic_rejects_read_before_first_publish() {
    SeqlockTopic<VehicleState> topic;
    VehicleState state = {};
    TEST_ASSERT_FALSE(topic.read(state));
}

void test_topic_publishes_consistent_snapshot() {
    SeqlockTopic<VehicleState> topic;
    VehicleState expected = {};
    expected.rollDeg = 12.5f;
    expected.timestampUs = 123456;
    expected.valid = true;
    topic.publish(expected);

    VehicleState actual = {};
    TEST_ASSERT_TRUE(topic.read(actual));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected.rollDeg, actual.rollDeg);
    TEST_ASSERT_EQUAL_UINT32(expected.timestampUs, actual.timestampUs);
    TEST_ASSERT_TRUE(actual.valid);
    TEST_ASSERT_EQUAL_UINT32(2, topic.sequence());
}

void test_topic_keeps_latest_value() {
    SeqlockTopic<RcInputState> topic;
    RcInputState state = {};
    for (uint16_t value = 1000; value < 1010; ++value) {
        state.aileron = value;
        topic.publish(state);
    }
    RcInputState actual = {};
    TEST_ASSERT_TRUE(topic.read(actual));
    TEST_ASSERT_EQUAL_UINT16(1009, actual.aileron);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_topic_rejects_read_before_first_publish);
    RUN_TEST(test_topic_publishes_consistent_snapshot);
    RUN_TEST(test_topic_keeps_latest_value);
    return UNITY_END();
}
