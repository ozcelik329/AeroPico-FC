#include <unity.h>

#include "hal/HAL_I2C.h"

class MockI2C final : public IHALI2C {
  public:
    void init(uint8_t sdaPin, uint8_t sclPin, uint32_t baudHz) override {
        lastSda = sdaPin;
        lastScl = sclPin;
        lastBaud = baudHz;
    }

    bool writeRaw(uint8_t address, const uint8_t* data, size_t length, bool nostop) override {
        lastAddress = address;
        lastWriteLength = length;
        lastNoStop = nostop;
        for (size_t i = 0; i < length && i < sizeof(lastWrite); i++) {
            lastWrite[i] = data[i];
        }
        return writeOk;
    }

    bool readRaw(uint8_t address, uint8_t* data, size_t length, bool nostop) override {
        lastAddress = address;
        lastReadLength = length;
        lastReadNoStop = nostop;
        for (size_t i = 0; i < length && i < sizeof(readData); i++) {
            data[i] = readData[i];
        }
        return readOk;
    }

    uint8_t lastSda = 0;
    uint8_t lastScl = 0;
    uint32_t lastBaud = 0;
    uint8_t lastAddress = 0;
    uint8_t lastWrite[4] = {};
    uint8_t readData[4] = {0xAA, 0x55, 0x12, 0x34};
    size_t lastWriteLength = 0;
    size_t lastReadLength = 0;
    bool lastNoStop = false;
    bool lastReadNoStop = true;
    bool writeOk = true;
    bool readOk = true;
};

void test_default_write_register_uses_two_byte_raw_write() {
    MockI2C bus;

    TEST_ASSERT_TRUE(bus.writeRegister(0x68, 0x6B, 0x00));

    TEST_ASSERT_EQUAL_UINT8(0x68, bus.lastAddress);
    TEST_ASSERT_EQUAL((size_t)2, bus.lastWriteLength);
    TEST_ASSERT_EQUAL_UINT8(0x6B, bus.lastWrite[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, bus.lastWrite[1]);
    TEST_ASSERT_FALSE(bus.lastNoStop);
}

void test_default_read_registers_uses_repeated_start() {
    MockI2C bus;
    uint8_t out[3] = {};

    TEST_ASSERT_TRUE(bus.readRegisters(0x1E, 0x03, out, sizeof(out)));

    TEST_ASSERT_EQUAL_UINT8(0x1E, bus.lastAddress);
    TEST_ASSERT_EQUAL((size_t)1, bus.lastWriteLength);
    TEST_ASSERT_EQUAL_UINT8(0x03, bus.lastWrite[0]);
    TEST_ASSERT_TRUE(bus.lastNoStop);
    TEST_ASSERT_EQUAL((size_t)3, bus.lastReadLength);
    TEST_ASSERT_FALSE(bus.lastReadNoStop);
    TEST_ASSERT_EQUAL_UINT8(0xAA, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0x55, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0x12, out[2]);
}

void test_default_read_registers_stops_when_address_write_fails() {
    MockI2C bus;
    uint8_t out[1] = {};
    bus.writeOk = false;

    TEST_ASSERT_FALSE(bus.readRegisters(0x77, 0xAA, out, sizeof(out)));
    TEST_ASSERT_EQUAL((size_t)0, bus.lastReadLength);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_default_write_register_uses_two_byte_raw_write);
    RUN_TEST(test_default_read_registers_uses_repeated_start);
    RUN_TEST(test_default_read_registers_stops_when_address_write_fails);
    return UNITY_END();
}
