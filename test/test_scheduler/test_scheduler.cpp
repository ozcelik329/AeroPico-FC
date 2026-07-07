#include <unity.h>

#include "core/Scheduler.h"

#include "../../src/core/Scheduler.cpp"

static int controlRuns;
static int telemetryRuns;
static int healthRuns;

static void runControl() { controlRuns++; }
static void runTelemetry() { telemetryRuns++; }
static void runHealth() { healthRuns++; }

void setUp() {
    controlRuns = 0;
    telemetryRuns = 0;
    healthRuns = 0;
}

void test_scheduler_runs_tasks_at_configured_rates() {
    Scheduler scheduler;
    scheduler.reset();
    TEST_ASSERT_TRUE(scheduler.addTask("control", 400, runControl));
    TEST_ASSERT_TRUE(scheduler.addTask("telemetry", 20, runTelemetry));
    TEST_ASSERT_TRUE(scheduler.addTask("health", 1, runHealth));

    for (uint32_t now = 0; now <= 1000000; now += 2500) {
        scheduler.tick(now);
    }

    TEST_ASSERT_EQUAL(400, controlRuns);
    TEST_ASSERT_EQUAL(20, telemetryRuns);
    TEST_ASSERT_EQUAL(1, healthRuns);
}

void test_scheduler_can_disable_task() {
    Scheduler scheduler;
    scheduler.reset();
    TEST_ASSERT_TRUE(scheduler.addTask("telemetry", 20, runTelemetry));
    TEST_ASSERT_TRUE(scheduler.setEnabled("telemetry", false));

    scheduler.tick(50000);
    TEST_ASSERT_EQUAL(0, telemetryRuns);

    TEST_ASSERT_TRUE(scheduler.setEnabled("telemetry", true));
    scheduler.tick(100000);
    TEST_ASSERT_EQUAL(1, telemetryRuns);
}

void test_scheduler_rejects_invalid_tasks() {
    Scheduler scheduler;
    scheduler.reset();

    TEST_ASSERT_FALSE(scheduler.addTask("bad_rate", 0, runControl));
    TEST_ASSERT_FALSE(scheduler.addTask("bad_callback", 10, nullptr));
    TEST_ASSERT_EQUAL((size_t)0, scheduler.taskCount());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_scheduler_runs_tasks_at_configured_rates);
    RUN_TEST(test_scheduler_can_disable_task);
    RUN_TEST(test_scheduler_rejects_invalid_tasks);
    return UNITY_END();
}
