#include <unity.h>

#include "drivers/gps/GpsManager.h"

#include "../../src/drivers/gps/GpsParser.cpp"
#include "../../src/drivers/gps/GpsManager.cpp"

class FakeGpsUart : public IHALUART {
  public:
    void begin(uint32_t baud) override {
        started = true;
        configuredBaud = baud;
    }
    int available() override {
        return sentence && sentence[index] != '\0' ? 1 : 0;
    }
    int read() override {
        if (!sentence || sentence[index] == '\0') return -1;
        return sentence[index++];
    }
    size_t write(uint8_t) override { return 1; }
    size_t write(const uint8_t*, size_t length) override { return length; }

    const char* sentence = nullptr;
    size_t index = 0;
    bool started = false;
    uint32_t configuredBaud = 0;
};

void test_gps_manager_reports_no_capability_until_valid_fix() {
    FakeGpsUart uart;
    GpsManager gps;
    gps.init(&uart, true, 9600);

    SensorCapabilityStatus capability = gps.capabilities();
    TEST_ASSERT_TRUE(uart.started);
    TEST_ASSERT_FALSE(capability.gpsAvailable);
    TEST_ASSERT_FALSE(hasSensorCapability(capability.functionMask, SENSOR_CAP_GPS));
}

void test_gps_manager_sets_gps_capability_after_fix() {
    FakeGpsUart uart;
    uart.sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
    GpsManager gps;
    gps.init(&uart, true, 9600);
    gps.update(1234);

    GpsStatus status = gps.status();
    SensorCapabilityStatus capability = gps.capabilities();
    TEST_ASSERT_TRUE(status.fix.valid);
    TEST_ASSERT_EQUAL_UINT32(1234, status.lastFixMs);
    TEST_ASSERT_TRUE(capability.gpsAvailable);
    TEST_ASSERT_TRUE(hasSensorCapability(capability.functionMask, SENSOR_CAP_GPS));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_gps_manager_reports_no_capability_until_valid_fix);
    RUN_TEST(test_gps_manager_sets_gps_capability_after_fix);
    return UNITY_END();
}
