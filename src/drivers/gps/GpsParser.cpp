#include "GpsParser.h"

#include <stdlib.h>
#include <string.h>

namespace {
static uint8_t fromHex(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    const char upper = (char)(c & ~0x20);
    if (upper >= 'A' && upper <= 'F') return (uint8_t)(10 + upper - 'A');
    return 0xFF;
}

static bool sentenceIsGga(const char* payload) {
    return payload &&
           payload[0] != '\0' && payload[1] != '\0' &&
           payload[2] == 'G' && payload[3] == 'G' &&
           payload[4] == 'A' && payload[5] == ',';
}
}

void GpsParser::reset() {
    _index = 0;
    _capturing = false;
}

bool GpsParser::push(char c, GpsFix& fix) {
    if (c == '$') {
        _capturing = true;
        _index = 0;
        _buffer[_index++] = c;
        return false;
    }
    if (!_capturing) return false;

    if (c == '\r') return false;
    if (c == '\n') {
        _buffer[_index] = '\0';
        _capturing = false;
        return parseSentence(fix);
    }
    if (_index >= MAX_SENTENCE - 1) {
        reset();
        return false;
    }
    _buffer[_index++] = c;
    return false;
}

bool GpsParser::parseSentence(GpsFix& fix) {
    _sentences++;
    uint8_t starIndex = 0xFF;
    for (uint8_t i = 0; _buffer[i] != '\0'; i++) {
        if (_buffer[i] == '*') {
            starIndex = i;
            break;
        }
    }
    if (starIndex == 0xFF || !checksumMatches(_buffer, starIndex)) {
        _checksumErrors++;
        return false;
    }

    _buffer[starIndex] = '\0';
    const char* payload = &_buffer[1];
    if (!sentenceIsGga(payload)) return false;
    return parseGga(payload, fix);
}

bool GpsParser::checksumMatches(const char* sentence, uint8_t starIndex) const {
    if (sentence[0] != '$' || sentence[starIndex + 1] == '\0' || sentence[starIndex + 2] == '\0') {
        return false;
    }
    uint8_t checksum = 0;
    for (uint8_t i = 1; i < starIndex; i++) {
        checksum ^= (uint8_t)sentence[i];
    }
    const uint8_t hi = fromHex(sentence[starIndex + 1]);
    const uint8_t lo = fromHex(sentence[starIndex + 2]);
    if (hi == 0xFF || lo == 0xFF) return false;
    return checksum == (uint8_t)((hi << 4) | lo);
}

bool GpsParser::parseGga(const char* payload, GpsFix& fix) const {
    const char* cursor = payload;
    char field[16];
    char lat[16];
    char ns[4];
    char lon[16];
    char ew[4];

    if (!nextField(cursor, field, sizeof(field))) return false; // sentence id
    if (!nextField(cursor, field, sizeof(field))) return false; // UTC
    if (!nextField(cursor, lat, sizeof(lat))) return false;
    if (!nextField(cursor, ns, sizeof(ns))) return false;
    if (!nextField(cursor, lon, sizeof(lon))) return false;
    if (!nextField(cursor, ew, sizeof(ew))) return false;
    if (!nextField(cursor, field, sizeof(field))) return false;
    const int fixQuality = atoi(field);
    if (!nextField(cursor, field, sizeof(field))) return false;
    const int satellites = atoi(field);
    if (!nextField(cursor, field, sizeof(field))) return false;
    const float hdop = strtof(field, nullptr);
    if (!nextField(cursor, field, sizeof(field))) return false;
    const float altitude = strtof(field, nullptr);

    if (fixQuality <= 0 || satellites <= 0) {
        fix.valid = false;
        return false;
    }
    if (lat[0] == '\0' || ns[0] == '\0' || lon[0] == '\0' || ew[0] == '\0') {
        fix.valid = false;
        return false;
    }

    fix.latitudeE7 = parseCoordinateE7(lat, ns);
    fix.longitudeE7 = parseCoordinateE7(lon, ew);
    fix.altitudeM = altitude;
    fix.hdopCm = (uint16_t)(hdop * 100.0f + 0.5f);
    fix.satellites = (uint8_t)(satellites > 255 ? 255 : satellites);
    fix.valid = true;
    return true;
}

bool GpsParser::nextField(const char*& cursor, char* out, size_t outLen) {
    if (!cursor || !out || outLen == 0) return false;
    size_t i = 0;
    while (*cursor != '\0' && *cursor != ',') {
        if (i + 1 < outLen) out[i++] = *cursor;
        cursor++;
    }
    out[i] = '\0';
    if (*cursor == ',') cursor++;
    return true;
}

int32_t GpsParser::parseCoordinateE7(const char* value, const char* hemi) {
    if (!value || !hemi || value[0] == '\0' || hemi[0] == '\0') return 0;
    const float raw = strtof(value, nullptr);
    const int degrees = (int)(raw / 100.0f);
    const float minutes = raw - (float)(degrees * 100);
    float decimalDegrees = (float)degrees + minutes / 60.0f;
    if (hemi[0] == 'S' || hemi[0] == 'W') decimalDegrees = -decimalDegrees;
    return (int32_t)(decimalDegrees * 10000000.0f + (decimalDegrees >= 0.0f ? 0.5f : -0.5f));
}
