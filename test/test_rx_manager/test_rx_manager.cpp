#include <unity.h>

#include "drivers/RX.h"

#include "../../src/drivers/RX.cpp"

class FakeSbusBackend : public ISbusBackend {
  public:
    SbusFrameView frame = {};
    bool hasFrame = false;
    int beginCount = 0;

    void begin() override { beginCount++; }

    bool readFrame(SbusFrameView& out) override {
        if (!hasFrame) {
            return false;
        }
        out = frame;
        hasFrame = false;
        return true;
    }
};

void test_rx_manager_uses_injected_sbus_backend() {
    FakeSbusBackend backend;
    backend.hasFrame = true;
    backend.frame.channels[RC_ROLL_CHANNEL] = 1811;
    backend.frame.channels[RC_PITCH_CHANNEL] = 172;
    backend.frame.channels[RC_THROTTLE_CHANNEL] = 992;
    backend.frame.channels[RC_YAW_CHANNEL] = 992;

    RXManager rx(&backend);
    rx.init();
    rx.update();

    TEST_ASSERT_EQUAL(1, backend.beginCount);
    TEST_ASSERT_TRUE(rx.isValid());
    TEST_ASSERT_EQUAL_UINT16(2000, rx.getChannel(RC_ROLL_CHANNEL));
    TEST_ASSERT_EQUAL_UINT16(1000, rx.getChannel(RC_PITCH_CHANNEL));
    TEST_ASSERT_EQUAL_UINT16(1500, rx.getChannel(RC_THROTTLE_CHANNEL));
}

void test_rx_manager_applies_protocol_failsafe() {
    FakeSbusBackend backend;
    backend.hasFrame = true;
    backend.frame.failsafe = true;

    RXManager rx(&backend);
    rx.init();
    rx.update();

    TEST_ASSERT_TRUE(rx.isFailsafe());
    TEST_ASSERT_FALSE(rx.isValid());
    TEST_ASSERT_EQUAL_UINT16(FAILSAFE_THROTTLE, rx.getChannel(RC_THROTTLE_CHANNEL));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_rx_manager_uses_injected_sbus_backend);
    RUN_TEST(test_rx_manager_applies_protocol_failsafe);
    return UNITY_END();
}
