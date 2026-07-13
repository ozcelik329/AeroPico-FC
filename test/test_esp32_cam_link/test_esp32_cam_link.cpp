#include <unity.h>

#include "drivers/camera/Esp32CamLink.h"

#include "../../src/drivers/camera/Esp32CamLink.cpp"

class FakeUart : public IHALUART {
  public:
    void begin(uint32_t baud) override {
        started = true;
        configuredBaud = baud;
    }
    int available() override { return availableBytes; }
    int read() override {
        if (availableBytes <= 0) return -1;
        availableBytes--;
        return 0x55;
    }
    size_t write(uint8_t) override { return 1; }
    size_t write(const uint8_t*, size_t length) override { return length; }

    bool started = false;
    uint32_t configuredBaud = 0;
    int availableBytes = 0;
};

void test_esp32_cam_link_stays_inert_when_disabled() {
    FakeUart uart;
    Esp32CamLink link;
    link.init(&uart, false, 115200);
    uart.availableBytes = 4;
    link.update(100);

    Esp32CamStatus status = link.status();
    TEST_ASSERT_FALSE(uart.started);
    TEST_ASSERT_FALSE(status.enabled);
    TEST_ASSERT_FALSE(status.linkActive);
    TEST_ASSERT_EQUAL_UINT32(0, status.bytesRx);
}

void test_esp32_cam_link_tracks_bytes_and_timeout() {
    FakeUart uart;
    Esp32CamLink link;
    link.init(&uart, true, 57600);
    uart.availableBytes = 3;
    link.update(100);

    Esp32CamStatus status = link.status();
    TEST_ASSERT_TRUE(uart.started);
    TEST_ASSERT_EQUAL_UINT32(57600, uart.configuredBaud);
    TEST_ASSERT_TRUE(status.enabled);
    TEST_ASSERT_TRUE(status.linkActive);
    TEST_ASSERT_EQUAL_UINT32(3, status.bytesRx);
    TEST_ASSERT_EQUAL_UINT32(100, status.lastByteMs);

    link.update(3201);
    status = link.status();
    TEST_ASSERT_FALSE(status.linkActive);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_esp32_cam_link_stays_inert_when_disabled);
    RUN_TEST(test_esp32_cam_link_tracks_bytes_and_timeout);
    return UNITY_END();
}
