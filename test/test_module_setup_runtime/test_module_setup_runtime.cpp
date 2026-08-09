#include <unity.h>

#include "core/safety/ModuleSetupRuntime.h"

#include "../../src/core/safety/ModuleSetupRuntime.cpp"

static ModuleSetupSnapshot validSnapshot() {
    ModuleSetupSnapshot snapshot = {};
    snapshot.baroEnabled = false;
    snapshot.magEnabled = false;
    snapshot.gpsEnabled = false;
    snapshot.batteryEnabled = false;
    snapshot.rcEnabled = false;
    snapshot.imuType = 1;
    snapshot.baroType = 1;
    snapshot.magType = 0;
    snapshot.gpsType = 1;
    snapshot.rcType = 1;
    snapshot.batteryType = 0;
    snapshot.i2cSda = 4;
    snapshot.i2cScl = 5;
    snapshot.buzzerPin = 22;
    snapshot.servoPins = {16, 17, 18, 19};
    snapshot.bootServoPinConfigValid = true;
    return snapshot;
}

void test_module_setup_accepts_valid_runtime_pin_map() {
    ModuleSetupRuntime runtime;
    runtime.update(validSnapshot());

    ModuleSetupEvaluation result = runtime.evaluate(0);

    TEST_ASSERT_TRUE(result.passed);
}

void test_module_setup_rejects_invalid_i2c_pair() {
    ModuleSetupRuntime runtime;
    ModuleSetupSnapshot snapshot = validSnapshot();
    snapshot.i2cSda = 8;
    snapshot.i2cScl = 5;
    runtime.update(snapshot);

    ModuleSetupEvaluation result = runtime.evaluate(0);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL_STRING("Setup I2C pin map invalid", result.reason);
}

void test_module_setup_rejects_buzzer_i2c_collision() {
    ModuleSetupRuntime runtime;
    ModuleSetupSnapshot snapshot = validSnapshot();
    snapshot.buzzerPin = 4;
    runtime.update(snapshot);

    ModuleSetupEvaluation result = runtime.evaluate(0);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL_STRING("Setup buzzer pin invalid", result.reason);
}

void test_module_setup_rejects_servo_buzzer_collision() {
    ModuleSetupRuntime runtime;
    ModuleSetupSnapshot snapshot = validSnapshot();
    snapshot.servoPins.aileron = 22;
    runtime.update(snapshot);

    ModuleSetupEvaluation result = runtime.evaluate(0);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL_STRING("Setup servo pin map invalid", result.reason);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_module_setup_accepts_valid_runtime_pin_map);
    RUN_TEST(test_module_setup_rejects_invalid_i2c_pair);
    RUN_TEST(test_module_setup_rejects_buzzer_i2c_collision);
    RUN_TEST(test_module_setup_rejects_servo_buzzer_collision);
    return UNITY_END();
}
