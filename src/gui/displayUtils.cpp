
#include "displayUtils.h"

#include "calendar/api.h"
#include "globals.h"

namespace gui {

String timeSpanStr(time_t startTime, time_t endTime) {
	return safeMyTZ.dateTime(startTime, UTC_TIME, "G:i") + " - "
	       + safeMyTZ.dateTime(endTime, UTC_TIME, "G:i");
}

bool active = true;

void sleepDisplay() {
	if (!active)
		return;
	active = false;
	M5.EPD.Sleep();
}

void wakeDisplay() {
	if (active)
		return;
	active = true;
	M5.EPD.Active();
}

};  // namespace gui
