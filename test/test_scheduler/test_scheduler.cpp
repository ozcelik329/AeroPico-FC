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
    TEST_ASSERT_TRUE(scheduler.addTask("control", 500, runControl));
    TEST_ASSERT_TRUE(scheduler.addTask("telemetry", 20, runTelemetry));
    TEST_ASSERT_TRUE(scheduler.addTask("health", 1, runHealth));

    for (uint32_t now = 0; now <= 1000000; now += 2000) {
        scheduler.tick(now);
    }

    TEST_ASSERT_EQUAL(500, controlRuns);
    TEST_ASSERT_EQUAL(20, telemetryRuns);
    TEST_ASSERT_EQUAL(1, healthRuns);
}

void test_scheduler_tracks_release_latency() {
    Scheduler scheduler;
    scheduler.reset();
    TEST_ASSERT_TRUE(scheduler.addTask("control", 500, runControl));

    scheduler.tick(2000);
    scheduler.tick(4300);

    const ScheduledTask* task = scheduler.getTask(0);
    TEST_ASSERT_NOT_NULL(task);
    TEST_ASSERT_EQUAL_UINT32(300, task->maxReleaseLatencyUs);
    TEST_ASSERT_EQUAL_UINT32(2, task->runCount);
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
    RUN_TEST(test_scheduler_tracks_release_latency);
    return UNITY_END();
}
