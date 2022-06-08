#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <ezTime.h>

namespace timeutils {

time_t getNextMidnight(time_t now);

time_t parseRfcTimestamp(const String& input);

}  // namespace timeutils

#endif