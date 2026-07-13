#ifndef GPS_PARSER_H
#define GPS_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include "GpsTypes.h"

class GpsParser {
  public:
    void reset();
    bool push(char c, GpsFix& fix);
    uint32_t sentences() const { return _sentences; }
    uint32_t checksumErrors() const { return _checksumErrors; }

  private:
    static constexpr uint8_t MAX_SENTENCE = 96;

    bool parseSentence(GpsFix& fix);
    bool checksumMatches(const char* sentence, uint8_t starIndex) const;
    bool parseGga(const char* payload, GpsFix& fix) const;
    static bool nextField(const char*& cursor, char* out, size_t outLen);
    static int32_t parseCoordinateE7(const char* value, const char* hemi);

    char _buffer[MAX_SENTENCE];
    uint8_t _index = 0;
    uint32_t _sentences = 0;
    uint32_t _checksumErrors = 0;
    bool _capturing = false;
};

#endif
