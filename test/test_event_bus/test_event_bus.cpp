#include <unity.h>

#include "core/events/SystemEventBus.h"

#include "../../src/core/events/SystemEventBus.cpp"

void setUp() {
    systemEventBus.reset();
}
void tearDown() {}

void test_event_bus_preserves_order() {
    TEST_ASSERT_TRUE(systemEventBus.publish({SystemEventType::RcLost, 10, 1}));
    TEST_ASSERT_TRUE(systemEventBus.publish({SystemEventType::FailsafeEntered, 20, 2}));

    SystemEvent event;
    TEST_ASSERT_TRUE(systemEventBus.consume(event));
    TEST_ASSERT_EQUAL_INT((int)SystemEventType::RcLost, (int)event.type);
    TEST_ASSERT_TRUE(systemEventBus.consume(event));
    TEST_ASSERT_EQUAL_INT((int)SystemEventType::FailsafeEntered, (int)event.type);
    TEST_ASSERT_FALSE(systemEventBus.consume(event));
}

void test_event_bus_counts_overflow() {
    for (uint8_t i = 0; i < SystemEventBus::CAPACITY - 1; ++i) {
        TEST_ASSERT_TRUE(systemEventBus.publish({SystemEventType::SensorFault, i, i}));
    }
    TEST_ASSERT_FALSE(systemEventBus.publish({SystemEventType::SensorFault, 99, 99}));
    TEST_ASSERT_EQUAL_UINT32(1, systemEventBus.droppedCount());
    TEST_ASSERT_EQUAL_UINT(SystemEventBus::CAPACITY - 1, systemEventBus.pendingCount());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_event_bus_preserves_order);
    RUN_TEST(test_event_bus_counts_overflow);
    return UNITY_END();
}
