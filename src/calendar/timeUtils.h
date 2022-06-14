#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <ezTime.h>

namespace timeutils {

time_t getNextMidnight(time_t now);

/**
 * Parses an RFC 3339 time string into a UTC time_t,
 * Not a full RFC 3339 parser, but common (and used by google) formats shown below are supported.
 * 2022-12-30T23:59:59Z
 * 2022-12-30T23:59:59+03:00
 * 2022-12-30T23:59:59-03:00
 * 2022-12-30T23:59:59.000Z
 * 2022-12-30T23:59:59.000+03:00
 * 2022-12-30T23:59:59.000-03:00
 * Seconds may contain any number of digits after the decimal point.
 */
time_t parseRfcTimestamp(const String& input);

}  // namespace timeutils

#endif