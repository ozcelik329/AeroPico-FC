#include <unity.h>

#include "storage/ParamStorage.h"

#include "../../src/storage/ParamStorage.cpp"

static ParamStorageBlob makeBlob() {
    float values[3] = {1.0f, 2.0f, 3.0f};
    return ParamStorage::makeBlob(values, 3);
}

void test_param_storage_rejects_bad_magic() {
    ParamStorageBlob blob = makeBlob();
    blob.magic ^= 0x55u;
    blob.checksum = ParamStorage::checksum(blob);

    TEST_ASSERT_FALSE(ParamStorage::isValid(blob, 3));
}

void test_param_storage_rejects_version_mismatch() {
    ParamStorageBlob blob = makeBlob();
    blob.version++;
    blob.checksum = ParamStorage::checksum(blob);

    TEST_ASSERT_FALSE(ParamStorage::isValid(blob, 3));
}

void test_param_storage_rejects_count_mismatch() {
    ParamStorageBlob blob = makeBlob();

    TEST_ASSERT_FALSE(ParamStorage::isValid(blob, 4));
}

void test_param_storage_rejects_checksum_mismatch() {
    ParamStorageBlob blob = makeBlob();
    blob.values[1] = 99.0f;

    TEST_ASSERT_FALSE(ParamStorage::isValid(blob, 3));
}

void test_memory_param_storage_round_trip() {
    MemoryParamStorage storage;
    ParamStorageBlob saved = makeBlob();
    ParamStorageBlob loaded = {};

    TEST_ASSERT_TRUE(storage.save(saved));
    TEST_ASSERT_TRUE(storage.load(loaded));
    TEST_ASSERT_TRUE(ParamStorage::isValid(loaded, 3));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, loaded.values[1]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_param_storage_rejects_bad_magic);
    RUN_TEST(test_param_storage_rejects_version_mismatch);
    RUN_TEST(test_param_storage_rejects_count_mismatch);
    RUN_TEST(test_param_storage_rejects_checksum_mismatch);
    RUN_TEST(test_memory_param_storage_round_trip);
    return UNITY_END();
}
