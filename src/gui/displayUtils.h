#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>

namespace gui {

String timeSpanStr(time_t startTime, time_t endTime);

};  // namespace gui

#endif