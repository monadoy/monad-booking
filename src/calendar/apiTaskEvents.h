#ifndef API_TASK_EVENTS_H
#define API_TASK_EVENTS_H

#include <esp_event.h>

#include "api.h"

namespace calevents {

esp_event_base_t const REQUEST = "calevents::REQUEST";

enum Request : uint32_t { REQUEST_CALENDAR_STATUS, REQUEST_INSERT, REQUEST_END };

struct RequestCalendarStatusData {};
struct RequestInsertData {
	time_t startTime;
	time_t endTime;
};
struct RequestEndData {
	String eventId;
};

esp_event_base_t const RESPONSE = "calevents::RESPONSE";

enum Response : uint32_t { RESPONSE_CALENDAR_STATUS, RESPONSE_INSERT, RESPONSE_END };

struct ResponseCalendarStatusData {
	cal::Result<cal::CalendarStatus> result;
};
struct ResponseInsertData {
	cal::Result<cal::Event> result;
};
struct ResponseEndData {
	cal::Result<cal::Event> result;
};

}  // namespace calevents

#endif