#define EZTIME_EZT_NAMESPACE 1
#include <ezTime.h>

#include "calendarApi.h"

// These variables need to be initialized to real values.
calapi::Token token{};
Timezone myTZ = UTC;
String calendarId = "";
String eventId = "";

// This is not meant to run, it only demonstrates available functions.
void init() {
	calapi::Result<calapi::CalendarStatus> statusRes
	    = calapi::fetchCalendarStatus(token, myTZ, calendarId);

	if (statusRes.isOk()) {
		auto ok = statusRes.ok();
		if (ok->currentEvent) {
			Serial.println("Result CURRENT EVENT: ");
			calapi::printEvent(*ok->currentEvent);
		}

		if (ok->nextEvent) {
			Serial.println("Result NEXT EVENT: ");
			calapi::printEvent(*ok->nextEvent);
		}
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(statusRes.err()->message);
	}

	// Insert a new event
	calapi::Result<calapi::Event> eventRes
	    = calapi::insertEvent(token, myTZ, calendarId, UTC.now(), UTC.now() + 30 * SECS_PER_MIN);

	if (eventRes.isOk()) {
		calapi::printEvent(*eventRes.ok());
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(eventRes.err()->message);
	}

	// End an event
	calapi::Result<calapi::Event> endedEventRes
	    = calapi::endEvent(token, myTZ, calendarId, eventId);

	if (endedEventRes.isOk()) {
		calapi::printEvent(*endedEventRes.ok());
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(endedEventRes.err()->message);
	}
}