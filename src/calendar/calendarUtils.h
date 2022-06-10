#ifndef CALENDAR_UTILS_H
#define CALENDAR_UTILS_H

#include "api.h"
#include "safeTimezone.h"

namespace calutils {

void printEvent(const cal::Event& event, SafeTimezone& tz);

}

#endif