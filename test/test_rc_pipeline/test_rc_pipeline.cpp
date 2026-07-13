#include <unity.h>

#include "core/rc/RCPipeline.h"

#include "../../src/core/rc/RCPipeline.cpp"

class FakeRxDriver : public IRxDriver {
  public:
    bool valid = true;
    bool failsafe = false;
    uint16_t channels[8] = {1500, 1500, 1000, 1500, 1500, 1500, 1500, 1500};
    int updateCount = 0;
    int initCount = 0;

    void init() override { initCount++; }
    void update() override { updateCount++; }
    bool isValid() const override { return valid; }
    bool isFailsafe() const override { return failsafe; }
    uint32_t lastValidMs() const override { return 0; }
    uint16_t getChannel(int ch) const override { return channels[ch]; }
};

void test_rc_pipeline_reads_receiver_channels() {
    FakeRxDriver rx;
    rx.channels[RC_ROLL_CHANNEL] = 1600;
    rx.channels[RC_PITCH_CHANNEL] = 1400;
    rx.channels[RC_THROTTLE_CHANNEL] = 1200;
    rx.channels[RC_YAW_CHANNEL] = 1700;
    rx.channels[RC_MODE_CHANNEL] = 1000;

    RCPipeline pipeline;
    pipeline.init(&rx);

    setMockMillis(50);
    RcInputState state = pipeline.update();

    TEST_ASSERT_EQUAL(1, rx.initCount);
    TEST_ASSERT_EQUAL(1, rx.updateCount);
    TEST_ASSERT_FALSE(state.failsafe);
    TEST_ASSERT_FALSE(state.overrideActive);
    TEST_ASSERT_EQUAL_UINT16(1600, state.aileron);
    TEST_ASSERT_EQUAL_UINT16(1400, state.elevator);
    TEST_ASSERT_EQUAL_UINT16(1200, state.throttle);
    TEST_ASSERT_EQUAL_UINT16(1700, state.rudder);
    TEST_ASSERT_EQUAL((int)ControlMode::Manual, (int)state.controlMode);
}

void test_rc_pipeline_uses_failsafe_values_when_rx_invalid() {
    FakeRxDriver rx;
    rx.valid = false;

    RCPipeline pipeline;
    pipeline.init(&rx);

    setMockMillis(75);
    RcInputState state = pipeline.update();

    TEST_ASSERT_TRUE(state.failsafe);
    TEST_ASSERT_FALSE(state.overrideActive);
    TEST_ASSERT_EQUAL_UINT16(PWM_NEUTRAL, state.aileron);
    TEST_ASSERT_EQUAL_UINT16(PWM_NEUTRAL, state.elevator);
    TEST_ASSERT_EQUAL_UINT16(PWM_MIN, state.throttle);
    TEST_ASSERT_EQUAL_UINT16(PWM_NEUTRAL, state.rudder);
    TEST_ASSERT_EQUAL((int)ControlMode::Manual, (int)state.controlMode);
}

void test_rc_pipeline_override_times_out() {
    FakeRxDriver rx;
    RCPipeline pipeline;
    pipeline.init(&rx);

    setMockMillis(100);
    pipeline.setOverride(1650, 1350, 1250, 1750);

    RcInputState active = pipeline.update();
    TEST_ASSERT_FALSE(active.failsafe);
    TEST_ASSERT_TRUE(active.overrideActive);
    TEST_ASSERT_EQUAL_UINT16(1650, active.aileron);

    setMockMillis(100 + MAVLINK_RC_OVERRIDE_TIMEOUT_MS + 1);
    RcInputState expired = pipeline.update();

    TEST_ASSERT_FALSE(expired.failsafe);
    TEST_ASSERT_FALSE(expired.overrideActive);
    TEST_ASSERT_EQUAL_UINT16(rx.channels[RC_ROLL_CHANNEL], expired.aileron);
}

void test_rc_pipeline_applies_runtime_channel_mapping() {
    FakeRxDriver rx;
    rx.channels[0] = 1100;
    rx.channels[1] = 1200;
    rx.channels[2] = 1300;
    rx.channels[3] = 1400;

    RCPipeline pipeline;
    pipeline.init(&rx);
    rx.channels[4] = 1000;
    rx.channels[5] = 1800;
    pipeline.applyMapping({3, 2, 1, 0, 5});

    setMockMillis(120);
    RcInputState state = pipeline.update();

    TEST_ASSERT_EQUAL_UINT16(1400, state.aileron);
    TEST_ASSERT_EQUAL_UINT16(1300, state.elevator);
    TEST_ASSERT_EQUAL_UINT16(1200, state.throttle);
    TEST_ASSERT_EQUAL_UINT16(1100, state.rudder);
    TEST_ASSERT_EQUAL((int)ControlMode::Stabilize, (int)state.controlMode);
}

void test_rc_pipeline_clamps_runtime_channel_mapping() {
    RCPipeline pipeline;
    pipeline.applyMapping({9, 8, 7, 6, 10});

    RcMapping mapping = pipeline.getMapping();

    TEST_ASSERT_EQUAL_UINT8(7, mapping.rollChannel);
    TEST_ASSERT_EQUAL_UINT8(7, mapping.pitchChannel);
    TEST_ASSERT_EQUAL_UINT8(7, mapping.throttleChannel);
    TEST_ASSERT_EQUAL_UINT8(6, mapping.yawChannel);
    TEST_ASSERT_EQUAL_UINT8(7, mapping.modeChannel);
}

void test_rc_pipeline_uses_mode_channel_for_control_mode() {
    FakeRxDriver rx;
    RCPipeline pipeline;
    pipeline.init(&rx);

    rx.channels[RC_MODE_CHANNEL] = RC_MODE_STABILIZE_THRESHOLD - 1;
    RcInputState manual = pipeline.update();
    TEST_ASSERT_EQUAL((int)ControlMode::Manual, (int)manual.controlMode);

    rx.channels[RC_MODE_CHANNEL] = RC_MODE_STABILIZE_THRESHOLD;
    RcInputState stabilize = pipeline.update();
    TEST_ASSERT_EQUAL((int)ControlMode::Stabilize, (int)stabilize.controlMode);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_rc_pipeline_reads_receiver_channels);
    RUN_TEST(test_rc_pipeline_uses_failsafe_values_when_rx_invalid);
    RUN_TEST(test_rc_pipeline_override_times_out);
    RUN_TEST(test_rc_pipeline_applies_runtime_channel_mapping);
    RUN_TEST(test_rc_pipeline_clamps_runtime_channel_mapping);
    RUN_TEST(test_rc_pipeline_uses_mode_channel_for_control_mode);
    return UNITY_END();
}
