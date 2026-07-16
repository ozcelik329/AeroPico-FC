#include <unity.h>

#include "telemetry/Blackbox.h"

#include "../../src/telemetry/Blackbox.cpp"

PioUart espUart;

void setUp() {
    espUart.bytesWritten = 0;
}
void tearDown() {}

void test_blackbox_crc16_known_vector() {
    static const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    TEST_ASSERT_EQUAL_HEX16(0x29B1, Blackbox::crc16(data, sizeof(data)));
}

void test_blackbox_writes_binary_flight_record() {
    blackbox.init();
    blackbox.setLogRateHz(50);
    setMockMillis(21);
    blackbox.log(1, 2, 3, 4, 5, 6, 1200, 1500, 1500, 1500, false, SensorHealth::Ok);
    TEST_ASSERT_EQUAL_UINT8(1, blackbox.queuedRecords());
    TEST_ASSERT_EQUAL(0, espUart.bytesWritten);
    TEST_ASSERT_EQUAL_UINT8(1, blackbox.drain());
    TEST_ASSERT_EQUAL(
        sizeof(BlackboxRecordHeader) + sizeof(BlackboxFlightPayload) + sizeof(uint16_t),
        espUart.bytesWritten
    );
    TEST_ASSERT_EQUAL_UINT32(0, blackbox.droppedRecords());
}

void test_blackbox_writes_runtime_health_record() {
    blackbox.init();
    RuntimeHealthStatus status = {};
    status.sensorStackHighWaterWords = 1000;
    status.flightStackHighWaterWords = 1200;
    status.telemetryStackHighWaterWords = 900;
    status.eventQueueDrops = 2;
    status.blackboxDrops = 3;

    blackbox.logRuntimeHealth(status);
    TEST_ASSERT_EQUAL_UINT8(1, blackbox.queuedRecords());
    TEST_ASSERT_EQUAL_UINT8(1, blackbox.drain());

    TEST_ASSERT_EQUAL(
        sizeof(BlackboxRecordHeader) + sizeof(RuntimeHealthStatus) + sizeof(uint16_t),
        espUart.bytesWritten
    );
    TEST_ASSERT_EQUAL_UINT32(0, blackbox.droppedRecords());
}

void test_blackbox_queue_backpressure_is_not_reported_as_data_corruption() {
    blackbox.init();
    blackbox.setLogRateHz(50);
    for (uint8_t i = 0; i < Blackbox::QUEUE_SLOTS + 2; ++i) {
        setMockMillis((uint32_t)(21 + i * 21));
        blackbox.log(1, 2, 3, 4, 5, 6, 1200, 1500, 1500, 1500, false, SensorHealth::Ok);
    }

    TEST_ASSERT_EQUAL_UINT32(0, blackbox.droppedRecords());
    TEST_ASSERT_TRUE(blackbox.backpressureRecords() > 0);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_blackbox_crc16_known_vector);
    RUN_TEST(test_blackbox_writes_binary_flight_record);
    RUN_TEST(test_blackbox_writes_runtime_health_record);
    RUN_TEST(test_blackbox_queue_backpressure_is_not_reported_as_data_corruption);
    return UNITY_END();
}
