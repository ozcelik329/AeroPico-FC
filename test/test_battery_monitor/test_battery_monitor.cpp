#include <unity.h>

#include "core/BatteryMonitor.h"

#include "../../src/core/BatteryMonitor.cpp"

static bool provideGoodVoltage(float& voltage) {
    voltage = 11.8f;
    return true;
}

static bool provideLowVoltage(float& voltage) {
    voltage = 9.2f;
    return true;
}

static bool provideNoVoltage(float& voltage) {
    voltage = 0.0f;
    return false;
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
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.8f, status.voltage);
}

void test_battery_monitor_rejects_low_voltage() {
    BatteryMonitor monitor;
    monitor.init(provideLowVoltage, 10.5f, 12.8f);

    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_TRUE(status.configured);
    TEST_ASSERT_FALSE(status.healthy);
}

void test_battery_monitor_rejects_unavailable_voltage() {
    BatteryMonitor monitor;
    monitor.init(provideNoVoltage, 10.5f, 12.8f);

    BatteryStatus status = monitor.evaluate();

    TEST_ASSERT_TRUE(status.configured);
    TEST_ASSERT_FALSE(status.healthy);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_battery_monitor_reports_not_configured);
    RUN_TEST(test_battery_monitor_accepts_voltage_in_range);
    RUN_TEST(test_battery_monitor_rejects_low_voltage);
    RUN_TEST(test_battery_monitor_rejects_unavailable_voltage);
    return UNITY_END();
}
