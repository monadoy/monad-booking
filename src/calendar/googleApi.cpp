#include "googleApi.h"

#include "globals.h"
#include "timeUtils.h"
#include "utils.h"

namespace cal {

namespace {
const char* EVENT_FIELDS = "id,creator,start,end,summary,attendees(resource,responseStatus)";

// Techincally we need to fetch only two events to gain knowledge of the current and next event.
// The problem is that the fetch returns declined events. Fetching too many events will make us run
// out of memory, so we need a some kind of sensible limit. Here we hope the at least two of these
// events are accepted.
const int LIST_MAX_EVENTS = 16;

// A single event takes around 600 bytes plus 50 per each additional attendee.
const int EVENT_MAX_SIZE = 1024;

const int EVENT_LIST_MAX_SIZE = LIST_MAX_EVENTS * EVENT_MAX_SIZE;
}  // namespace

GoogleAPI::GoogleAPI(const Token& token, const String& calendarId)
    : _token{token}, _calendarId{calendarId} {
	_http.setReuse(false);
};

bool GoogleAPI::refreshAuth() {
	log_i("Refreshing token...");
	if (_token.unixExpiry + 60 > safeUTC.now()) {
		log_i("Token doesn't need refreshing");
		return true;
	}

	// BUILD REQUEST
	_http.begin("https://oauth2.googleapis.com/token");
	_http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	// SEND REQUEST
	int httpCode
	    = _http.POST("client_id=" + _token.clientId + "&client_secret=" + _token.clientSecret
	                 + "&refresh_token=" + _token.refreshToken + "&grant_type=refresh_token");

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	// log_i("Received refresh auth response:\n%s", responseBody.c_str());
	log_i("Received refresh auth response: hidden");
	DynamicJsonDocument doc(EVENT_LIST_MAX_SIZE);
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return false;

	// PARSE JSON VALUES INTO TOKEN
	_token.accessToken = doc["access_token"].as<String>();
	_token.unixExpiry = safeUTC.now() + doc["expires_in"].as<long>();
	assert(_saveTokenFunc);
	if (doc.containsKey("refresh_token")) {
		_token.refreshToken = doc["refresh_token"].as<String>();
		_saveTokenFunc(_token);
	}

	return true;
};

Result<CalendarStatus> GoogleAPI::fetchCalendarStatus() {
	// BUILD REQUEST
	time_t now = safeMyTZ.now();

	String timeMin = safeMyTZ.dateTime(now, RFC3339);
	timeMin.replace("+", "%2b");

	String timeMax = safeMyTZ.dateTime(timeutils::getNextMidnight(now), RFC3339);
	timeMax.replace("+", "%2b");

	String timeZone = safeMyTZ.getOlson();

	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId
	             + "/events?timeMin=" + timeMin + "&timeMax=" + timeMax + "&timeZone=" + timeZone
	             + "&maxResults=" + LIST_MAX_EVENTS
	             + "&maxAttendees=1&singleEvents=true&orderBy=startTime&fields=summary,items("
	             + EVENT_FIELDS + ")";

	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// SEND REQUEST
	int httpCode = _http.GET();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	log_i("Received event list response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(EVENT_LIST_MAX_SIZE);
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<CalendarStatus>::makeErr(err);

	// PARSE JSON AS CALENDAR STATUS STRUCT
	auto status = new CalendarStatus{
	    .name = "",
	    .currentEvent = nullptr,
	    .nextEvent = nullptr,
	};

	status->name = doc["summary"].as<String>();
	JsonArrayConst items = doc["items"].as<JsonArrayConst>();
	now = safeUTC.now();

	for (JsonObjectConst item : items) {
		std::shared_ptr<Event> event = extractEvent(item);

		if (!event)
			continue;

		if (event->unixStartTime <= now && now <= event->unixEndTime
		    && status->currentEvent == nullptr) {
			status->currentEvent = std::move(event);
		} else if (event->unixStartTime > now && status->nextEvent == nullptr) {
			status->nextEvent = std::move(event);
		}
	}

	return Result<CalendarStatus>::makeOk(status);
};

Result<Event> GoogleAPI::endEvent(const String& eventId) {
	// BUILD REQUEST
	String nowStr = safeMyTZ.dateTime(RFC3339);
	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId + "/events/"
	             + eventId + "?fields=" + EVENT_FIELDS;
	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// CREATE PAYLOAD
	StaticJsonDocument<256> payloadDoc;
	payloadDoc["end"]["dateTime"] = nowStr;
	payloadDoc["end"]["timeZone"] = safeMyTZ.getOlson();
	String payload = "";
	serializeJson(payloadDoc, payload);

	// SEND REQUEST
	int httpCode = _http.PATCH(payload);
	payload.clear();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();

	log_i("Received event patch response:\n%s", responseBody.c_str());
	StaticJsonDocument<1024> doc;
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<Event>::makeErr(err);

	// PARSE JSON AS EVENT STRUCT
	std::shared_ptr<Event> event = extractEvent(doc.as<JsonObject>());
	if (!event) {
		return Result<Event>::makeErr(
		    new Error{Error::Type::LOGICAL, "PATCH didn't return a valid accepted event"});
	}

	return Result<Event>::makeOk(event);
}

Result<Event> GoogleAPI::insertEvent(time_t startTime, time_t endTime) {
	// Check that insertion is possible
	Result<bool> isFreeRes = isFree(startTime, endTime);
	if (isFreeRes.isErr())
		return Result<Event>::makeErr(isFreeRes.err());
	if (*isFreeRes.ok() == false) {
		return Result<Event>::makeErr(new Error(
		    Error::Type::LOGICAL, "Couldn't insert, it would overlap with another event"));
	}

	// BUILD REQUEST
	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId
	             + "/events?fields=" + EVENT_FIELDS;
	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// CREATE PAYLOAD
	StaticJsonDocument<256> payloadDoc;
	payloadDoc["start"]["dateTime"] = safeMyTZ.dateTime(startTime, UTC_TIME, RFC3339);
	payloadDoc["start"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["end"]["dateTime"] = safeMyTZ.dateTime(endTime, UTC_TIME, RFC3339);
	payloadDoc["end"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["summary"] = l10n.msg(L10nMessage::NEW_EVENT_SUMMARY);
	String payload = "";
	serializeJson(payloadDoc, payload);

	log_i("Sending event insert payload:\n%s", payload.c_str());

	// SEND REQUEST
	int httpCode = _http.POST(payload);
	payload.clear();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();

	log_i("Received event insert response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(1024);
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<Event>::makeErr(err);

	// PARSE JSON AS EVENT STRUCT
	std::shared_ptr<Event> event = extractEvent(doc.as<JsonObject>(), true);
	if (!event) {
		return Result<Event>::makeErr(
		    new Error{Error::Type::LOGICAL, "INSERT didn't return a valid event"});
	}

	return Result<Event>::makeOk(event);
}

Result<Event> GoogleAPI::rescheduleEvent(std::shared_ptr<Event> event, time_t newStartTime,
                                         time_t newEndTime) {
	// Check that reschedule is possible
	Result<bool> isFreeRes = isFree(newStartTime, newEndTime, event->id);
	if (isFreeRes.isErr())
		return Result<Event>::makeErr(isFreeRes.err());
	if (*isFreeRes.ok() == false) {
		return Result<Event>::makeErr(new Error(
		    Error::Type::LOGICAL, "Couldn't reschedule, it would overlap with another event"));
	}

	// BUILD REQUEST
	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId + "/events/"
	             + event->id + "?fields=" + EVENT_FIELDS;
	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// CREATE PAYLOAD
	StaticJsonDocument<256> payloadDoc;
	payloadDoc["start"]["dateTime"] = safeMyTZ.dateTime(newStartTime, UTC_TIME, RFC3339);
	payloadDoc["start"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["end"]["dateTime"] = safeMyTZ.dateTime(newEndTime, UTC_TIME, RFC3339);
	payloadDoc["end"]["timeZone"] = safeMyTZ.getOlson();
	String payload = "";
	serializeJson(payloadDoc, payload);

	// SEND REQUEST
	int httpCode = _http.PATCH(payload);
	payload.clear();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();

	log_i("Received event patch response:\n%s", responseBody.c_str());
	StaticJsonDocument<1024> doc;
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<Event>::makeErr(err);

	// PARSE JSON AS EVENT STRUCT
	std::shared_ptr<Event> newEvent = extractEvent(doc.as<JsonObject>(), true);
	if (!newEvent) {
		return Result<Event>::makeErr(
		    new Error{Error::Type::LOGICAL, "PATCH didn't return a valid event"});
	}

	return Result<Event>::makeOk(newEvent);
}

bool GoogleAPI::isRoomAccepted(JsonObjectConst eventObject) {
	JsonArrayConst attendees = eventObject["attendees"].as<JsonArrayConst>();
	for (JsonObjectConst attendee : attendees) {
		if ((attendee["resource"] | false) == true && attendee["responseStatus"] == "accepted") {
			return true;
		}
	}
	return false;
}

Result<bool> GoogleAPI::isFree(time_t startTime, time_t endTime, const String& ignoreId) {
	// BUILD REQUEST
	String timeMin = safeMyTZ.dateTime(startTime, UTC_TIME, RFC3339);
	timeMin.replace("+", "%2b");
	String timeMax = safeMyTZ.dateTime(endTime, UTC_TIME, RFC3339);
	timeMax.replace("+", "%2b");
	String timeZone = safeMyTZ.getOlson();

	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId
	             + "/events?timeMin=" + timeMin + "&timeMax=" + timeMax + "&timeZone=" + timeZone
	             + "&maxResults=" + LIST_MAX_EVENTS
	             + "&maxAttendees=1&singleEvents=true&orderBy=startTime"
	             + "&fields=items(id,attendees(resource,responseStatus))";

	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// SEND REQUEST
	int httpCode = _http.GET();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	log_i("Received event isFree response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(EVENT_LIST_MAX_SIZE);
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<bool>::makeErr(err);

	JsonArrayConst items = doc["items"].as<JsonArrayConst>();

	for (JsonObjectConst item : items) {
		if (!isRoomAccepted(item))
			continue;

		if (item["id"].as<String>() == ignoreId)
			continue;

		// Is not free, because an accepted event overlaps
		// with timespan between startTime and endTime
		return Result<bool>::makeOk(new bool(false));
	}

	return Result<bool>::makeOk(new bool(true));
}

std::shared_ptr<Event> GoogleAPI::extractEvent(JsonObjectConst object, bool ignoreRoomAccept) {
	// Whole day events contain "date" key, ignore these for now
	if (object["start"].containsKey("date"))
		return nullptr;

	// Check that this room has accepted the event.
	// It will not be accepted if it would overlap with another event in this room.
	if (!ignoreRoomAccept && !isRoomAccepted(object))
		return nullptr;

	// Extract the values from the JSON object

	time_t startTime = timeutils::parseRfcTimestamp(object["start"]["dateTime"]);
	time_t endTime = timeutils::parseRfcTimestamp(object["end"]["dateTime"]);

	return std::shared_ptr<Event>(new Event{
	    .id = object["id"],
	    .creator = object["creator"]["displayName"] | object["creator"]["email"],
	    .summary = object["summary"] | "(No title)",
	    .unixStartTime = startTime,
	    .unixEndTime = endTime,
	});
}

}  // namespace cal