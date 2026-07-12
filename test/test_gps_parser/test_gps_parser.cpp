#include <unity.h>

#include "drivers/gps/GpsParser.h"

#include "../../src/drivers/gps/GpsParser.cpp"

void test_gps_parser_accepts_valid_gga_fix() {
    GpsParser parser;
    parser.reset();
    GpsFix fix = {};
    const char* sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
    bool parsed = false;
    for (const char* p = sentence; *p; p++) {
        parsed = parser.push(*p, fix) || parsed;
    }

    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_TRUE(fix.valid);
    TEST_ASSERT_INT32_WITHIN(32, 481173000, fix.latitudeE7);
    TEST_ASSERT_INT32_WITHIN(32, 115166667, fix.longitudeE7);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 545.4f, fix.altitudeM);
    TEST_ASSERT_EQUAL_UINT8(8, fix.satellites);
    TEST_ASSERT_EQUAL_UINT16(90, fix.hdopCm);
}

void test_gps_parser_rejects_bad_checksum() {
    GpsParser parser;
    parser.reset();
    GpsFix fix = {};
    const char* sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00\n";
    bool parsed = false;
    for (const char* p = sentence; *p; p++) {
        parsed = parser.push(*p, fix) || parsed;
    }

    TEST_ASSERT_FALSE(parsed);
    TEST_ASSERT_FALSE(fix.valid);
    TEST_ASSERT_EQUAL_UINT32(1, parser.checksumErrors());
}

void test_gps_parser_rejects_no_fix_gga() {
    GpsParser parser;
    parser.reset();
    GpsFix fix = {};
    const char* sentence = "$GPGGA,123519,4807.038,N,01131.000,E,0,00,9.9,545.4,M,46.9,M,,*48\n";
    bool parsed = false;
    for (const char* p = sentence; *p; p++) {
        parsed = parser.push(*p, fix) || parsed;
    }

    TEST_ASSERT_FALSE(parsed);
    TEST_ASSERT_FALSE(fix.valid);
}

void test_gps_parser_accepts_lowercase_hex_checksum() {
    GpsParser parser;
    parser.reset();
    GpsFix fix = {};
    const char* sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,108.4,M,46.9,M,,*4a\n";
    bool parsed = false;
    for (const char* p = sentence; *p; p++) {
        parsed = parser.push(*p, fix) || parsed;
    }

    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_TRUE(fix.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 108.4f, fix.altitudeM);
}

void test_gps_parser_rejects_missing_coordinates_even_with_fix_quality() {
    GpsParser parser;
    parser.reset();
    GpsFix fix = {};
    const char* sentence = "$GPGGA,123519,,,,,1,08,0.9,545.4,M,46.9,M,,*7e\n";
    bool parsed = false;
    for (const char* p = sentence; *p; p++) {
        parsed = parser.push(*p, fix) || parsed;
    }

    TEST_ASSERT_FALSE(parsed);
    TEST_ASSERT_FALSE(fix.valid);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_gps_parser_accepts_valid_gga_fix);
    RUN_TEST(test_gps_parser_rejects_bad_checksum);
    RUN_TEST(test_gps_parser_rejects_no_fix_gga);
    RUN_TEST(test_gps_parser_accepts_lowercase_hex_checksum);
    RUN_TEST(test_gps_parser_rejects_missing_coordinates_even_with_fix_quality);
    return UNITY_END();
}
