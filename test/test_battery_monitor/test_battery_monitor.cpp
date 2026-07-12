#include <unity.h>

#include "core/safety/BatteryMonitor.h"

#include "../../src/core/safety/BatteryMonitor.cpp"

static bool provideGoodVoltage(float& voltage) {
    voltage = 11.8f;
    return true;
}

static bool provideLowVoltage(float& voltage) {
    voltage = 9.2f;
    return true;
}

static bool provideBrownoutVoltage(float& voltage) {
    voltage = 8.8f;
    return true;
}

static bool provideNoVoltage(float& voltage) {
    voltage = 0.0f;
    return false;
}

static float scriptedVoltage = 12.0f;

static bool provideScriptedVoltage(float& voltage) {
    voltage = scriptedVoltage;
    return true;
}

void test_battery_monitor_reports_not_configured() {
    BatteryMonitor monitor;
    monitor.init();

    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_FALSE(status.configured);
    TEST_ASSERT_FALSE(status.healthy);
}

void test_battery_monitor_accepts_voltage_in_range() {
    BatteryMonitor monitor;
    monitor.init(provideGoodVoltage, 10.5f, 12.8f);

    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_TRUE(status.configured);
    TEST_ASSERT_TRUE(status.healthy);
    TEST_ASSERT_FALSE(status.brownout);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.8f, status.voltage);
}

void test_battery_monitor_rejects_low_voltage() {
    BatteryMonitor monitor;
    monitor.init(provideLowVoltage, 10.5f, 12.8f);

    monitor.evaluate();
    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_TRUE(status.configured);
    TEST_ASSERT_FALSE(status.healthy);
    TEST_ASSERT_TRUE(status.low);
    TEST_ASSERT_FALSE(status.brownout);
}

void test_battery_monitor_rejects_unavailable_voltage() {
    BatteryMonitor monitor;
    monitor.init(provideNoVoltage, 10.5f, 12.8f);

    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_TRUE(status.configured);
    TEST_ASSERT_FALSE(status.healthy);
    TEST_ASSERT_FALSE(status.brownout);
}

void test_battery_monitor_flags_brownout_risk() {
    BatteryMonitor monitor;
    monitor.init(provideBrownoutVoltage, 10.5f, 12.8f, 9.0f);

    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_TRUE(status.configured);
    TEST_ASSERT_FALSE(status.healthy);
    TEST_ASSERT_TRUE(status.brownout);
}

void test_battery_monitor_debounces_low_voltage() {
    BatteryMonitor monitor;
    scriptedVoltage = 10.3f;
    monitor.init(provideScriptedVoltage, 10.5f, 12.8f, 9.0f);

    BatteryStatus first = monitor.evaluate();
    BatteryStatus second = monitor.evaluate();

    TEST_ASSERT_TRUE(first.healthy);
    TEST_ASSERT_FALSE(first.low);
    TEST_ASSERT_FALSE(second.healthy);
    TEST_ASSERT_TRUE(second.low);
}

void test_battery_monitor_uses_brownout_recovery_hysteresis() {
    BatteryMonitor monitor;
    scriptedVoltage = 8.8f;
    monitor.init(provideScriptedVoltage, 10.5f, 12.8f, 9.0f);

    BatteryStatus brownout = monitor.evaluate();
    scriptedVoltage = 9.2f;
    BatteryStatus stillLatched = monitor.evaluate();

    TEST_ASSERT_TRUE(brownout.brownout);
    TEST_ASSERT_TRUE(stillLatched.brownout);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_battery_monitor_reports_not_configured);
    RUN_TEST(test_battery_monitor_accepts_voltage_in_range);
    RUN_TEST(test_battery_monitor_rejects_low_voltage);
    RUN_TEST(test_battery_monitor_rejects_unavailable_voltage);
    RUN_TEST(test_battery_monitor_flags_brownout_risk);
    RUN_TEST(test_battery_monitor_debounces_low_voltage);
    RUN_TEST(test_battery_monitor_uses_brownout_recovery_hysteresis);
    return UNITY_END();
}
