#include <unity.h>

#include "app/ServiceCommandMailbox.h"

#include "../../src/app/ServiceCommandMailbox.cpp"

void test_mailbox_submits_and_claims_single_command() {
    ServiceCommandMailbox mailbox;
    ServiceCommandRequest request = {4, 1.0f, 1500.0f, 500.0f};
    TEST_ASSERT_TRUE(mailbox.submit(request));

    ServiceCommandRequest claimed = {};
    TEST_ASSERT_TRUE(mailbox.claim(claimed));
    TEST_ASSERT_EQUAL_UINT16(request.action, claimed.action);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, request.p3, claimed.p3);
    TEST_ASSERT_FALSE(mailbox.claim(claimed));
}

void test_mailbox_rejects_second_pending_command() {
    ServiceCommandMailbox mailbox;
    TEST_ASSERT_TRUE(mailbox.submit({1, 0.0f, 0.0f, 0.0f}));
    TEST_ASSERT_FALSE(mailbox.submit({2, 0.0f, 0.0f, 0.0f}));
}

void test_mailbox_completion_round_trip() {
    ServiceCommandMailbox mailbox;
    mailbox.complete(1, 0, "done");

    ServiceCommandCompletion completion = {};
    TEST_ASSERT_TRUE(mailbox.takeCompletion(completion));
    TEST_ASSERT_EQUAL_UINT16(1, completion.action);
    TEST_ASSERT_EQUAL_UINT8(0, completion.result);
    TEST_ASSERT_EQUAL_STRING("done", completion.reason);
    TEST_ASSERT_FALSE(mailbox.takeCompletion(completion));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mailbox_submits_and_claims_single_command);
    RUN_TEST(test_mailbox_rejects_second_pending_command);
    RUN_TEST(test_mailbox_completion_round_trip);
    return UNITY_END();
}
