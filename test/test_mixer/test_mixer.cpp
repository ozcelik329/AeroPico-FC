#include <unity.h>

#include "core/control/FixedWingMixer.h"

#include "../../src/core/control/FixedWingMixer.cpp"

void test_mixer_neutral_outputs_stay_neutral() {
    FixedWingMixer mixer;
    MixerOutput out = mixer.computeOutputs(1000, 0.0f, 0.0f, 0.0f, 1500, 1500, 1500);

    TEST_ASSERT_EQUAL_INT(PWM_MIN, out.throttle);
    TEST_ASSERT_EQUAL_INT(PWM_NEUTRAL, out.aileron);
    TEST_ASSERT_EQUAL_INT(PWM_NEUTRAL, out.elevator);
    TEST_ASSERT_EQUAL_INT(PWM_NEUTRAL, out.rudder);
}

void test_mixer_clamps_servo_outputs() {
    FixedWingMixer mixer;
    MixerOutput out = mixer.computeOutputs(2500, 2000.0f, -2000.0f, 2000.0f, 1500, 1500, 1500);

    TEST_ASSERT_EQUAL_INT(PWM_MAX, out.throttle);
    TEST_ASSERT_EQUAL_INT(PWM_MAX, out.aileron);
    TEST_ASSERT_EQUAL_INT(PWM_MIN, out.elevator);
    TEST_ASSERT_EQUAL_INT(PWM_MAX, out.rudder);
}

void test_mixer_applies_trim_and_gains() {
    FixedWingMixer mixer;
    MixerSettings settings;
    settings.rollGain = 0.5f;
    settings.pitchGain = 1.0f;
    settings.yawGain = 1.0f;
    settings.aileronTrim = 10;
    settings.elevatorTrim = -10;
    settings.rudderTrim = 0;
    settings.throttleTrim = 20;
    settings.servoMin = PWM_MIN;
    settings.servoMax = PWM_MAX;
    settings.reverseAileron = false;
    settings.reverseElevator = false;
    settings.reverseRudder = false;
    mixer.setSettings(settings);

    MixerOutput out = mixer.computeOutputs(1200, 100.0f, 100.0f, 0.0f, 1500, 1500, 1500);

    TEST_ASSERT_EQUAL_INT(1220, out.throttle);
    TEST_ASSERT_EQUAL_INT(1560, out.aileron);
    TEST_ASSERT_EQUAL_INT(1590, out.elevator);
    TEST_ASSERT_EQUAL_INT(1500, out.rudder);
}

void test_mixer_applies_reverse_and_servo_limits() {
    FixedWingMixer mixer;
    MixerSettings settings;
    settings.rollGain = 1.0f;
    settings.pitchGain = 1.0f;
    settings.yawGain = 1.0f;
    settings.aileronTrim = 0;
    settings.elevatorTrim = 0;
    settings.rudderTrim = 0;
    settings.throttleTrim = 0;
    settings.servoMin = 1100;
    settings.servoMax = 1900;
    settings.reverseAileron = true;
    settings.reverseElevator = false;
    settings.reverseRudder = false;
    mixer.setSettings(settings);

    MixerOutput out = mixer.computeOutputs(1200, 100.0f, -600.0f, 0.0f, 1500, 1500, 1500);

    TEST_ASSERT_EQUAL_INT(1400, out.aileron);
    TEST_ASSERT_EQUAL_INT(1100, out.elevator);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mixer_neutral_outputs_stay_neutral);
    RUN_TEST(test_mixer_clamps_servo_outputs);
    RUN_TEST(test_mixer_applies_trim_and_gains);
    RUN_TEST(test_mixer_applies_reverse_and_servo_limits);
    return UNITY_END();
}
