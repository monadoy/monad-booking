
#include "displayUtils.h"

#include "calendar/api.h"
#include "globals.h"

namespace gui {

String timeSpanStr(time_t startTime, time_t endTime) {
	return safeMyTZ.dateTime(startTime, UTC_TIME, "G:i") + " - "
	       + safeMyTZ.dateTime(endTime, UTC_TIME, "G:i");
}

};  // namespace gui
