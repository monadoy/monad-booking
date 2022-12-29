#include "microsoftApi.h"

#include "globals.h"
#include "timeUtils.h"
#include "utils.h"

namespace cal {

namespace {
const int EVENT_LIST_MAX_SIZE = 4096;
const char* EVENT_FIELDS = "id,subject,organizer,start,end";
const int NAME_GET_MAX_SIZE = 1024;
}  // namespace

MicrosoftAPI::MicrosoftAPI(const Token& token, const String& calendarId)
    : _token{token}, _roomEmail{calendarId} {
	_http.setReuse(false);
};

const int AUTH_RESPONSE_MAX_SIZE = 4096;

bool MicrosoftAPI::refreshAuth() {
	log_i("Refreshing token...");
	if (_token.unixExpiry + 60 > safeUTC.now()) {
		log_i("Token doesn't need refreshing");
		return true;
	}

	// BUILD REQUEST
	_http.begin("https://login.microsoftonline.com/organizations/oauth2/v2.0/token");
	_http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	// SEND REQUEST
	int httpCode = _http.POST("client_id=" + _token.clientId + "&refresh_token="
	                          + _token.refreshToken + "&grant_type=refresh_token");

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	// log_i("Received refresh auth response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(AUTH_RESPONSE_MAX_SIZE);
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

Result<CalendarStatus> MicrosoftAPI::fetchCalendarStatus() {
	// Get room name if not already set
	if (_roomName.length() == 0) {
		auto res = getRoomName();
		if (res.isErr())
			return Result<CalendarStatus>::makeErr(res.err());
		_roomName = *res.ok();
	}

	// BUILD REQUEST
	time_t now = safeMyTZ.now();

	String timeMin = safeMyTZ.dateTime(now, RFC3339);
	timeMin.replace("+", "%2b");

	String timeMax = safeMyTZ.dateTime(timeutils::getNextMidnight(now), RFC3339);
	timeMax.replace("+", "%2b");

	String timeZone = safeMyTZ.getOlson();

	String url = "https://graph.microsoft.com/v1.0/users/" + _roomEmail
	             + "/calendarView?startDateTime=" + timeMin + "&endDateTime=" + timeMax
	             + "&$select=" + EVENT_FIELDS + "&$orderby=start/dateTime&$top=2";

	_http.begin(url);
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);
	_http.addHeader("Prefer", "outlook.timezone=\"UTC\"");

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
	    .name = _roomName,
	    .currentEvent = nullptr,
	    .nextEvent = nullptr,
	};

	JsonArrayConst items = doc["value"].as<JsonArrayConst>();
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

Result<Event> MicrosoftAPI::endEvent(const String& eventId) {
	/// BUILD REQUEST
	String nowStr = safeMyTZ.dateTime(RFC3339);
	String url = "https://graph.microsoft.com/v1.0/users/" + _roomEmail + "/events/" + eventId
	             + "?$select=" + EVENT_FIELDS;
	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);
	_http.addHeader("Prefer", "outlook.timezone=\"UTC\"");

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

Result<Event> MicrosoftAPI::insertEvent(time_t startTime, time_t endTime) {
	// Check that insertion is possible
	Result<bool> isFreeRes = isFree(startTime, endTime);
	if (isFreeRes.isErr())
		return Result<Event>::makeErr(isFreeRes.err());
	if (*isFreeRes.ok() == false) {
		return Result<Event>::makeErr(new Error(
		    Error::Type::LOGICAL, "Couldn't insert, it would overlap with another event"));
	}

	// BUILD REQUEST
	String url = "https://graph.microsoft.com/v1.0/users/" + _roomEmail
	             + "/events/?$select=" + String(EVENT_FIELDS);
	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Prefer", "outlook.timezone=\"UTC\"");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// CREATE PAYLOAD
	StaticJsonDocument<256> payloadDoc;
	payloadDoc["start"]["dateTime"] = safeMyTZ.dateTime(startTime, UTC_TIME, RFC3339);
	payloadDoc["start"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["end"]["dateTime"] = safeMyTZ.dateTime(endTime, UTC_TIME, RFC3339);
	payloadDoc["end"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["subject"] = l10n.msg(L10nMessage::NEW_EVENT_SUMMARY);
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
	std::shared_ptr<Event> event = extractEvent(doc.as<JsonObject>());
	if (!event) {
		return Result<Event>::makeErr(
		    new Error{Error::Type::LOGICAL, "INSERT didn't return a valid event"});
	}

	return Result<Event>::makeOk(event);
}

Result<Event> MicrosoftAPI::rescheduleEvent(std::shared_ptr<Event> event, time_t newStartTime,
                                            time_t newEndTime) {
	// Check that reschedule is possible
	Result<bool> isFreeRes = isFree(newStartTime, newEndTime, event->id);
	if (isFreeRes.isErr())
		return Result<Event>::makeErr(isFreeRes.err());
	if (*isFreeRes.ok() == false) {
		return Result<Event>::makeErr(new Error(
		    Error::Type::LOGICAL, "Couldn't reschedule, it would overlap with another event"));
	}

	/// BUILD REQUEST
	String nowStr = safeMyTZ.dateTime(RFC3339);
	String url = "https://graph.microsoft.com/v1.0/users/" + _roomEmail + "/events/" + event->id
	             + "?$select=" + EVENT_FIELDS;
	_http.begin(url);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);
	_http.addHeader("Prefer", "outlook.timezone=\"UTC\"");

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
	std::shared_ptr<Event> newEvent = extractEvent(doc.as<JsonObject>());
	if (!newEvent) {
		return Result<Event>::makeErr(
		    new Error{Error::Type::LOGICAL, "PATCH didn't return a valid event"});
	}

	return Result<Event>::makeOk(newEvent);
}

Result<bool> MicrosoftAPI::isFree(time_t startTime, time_t endTime, const String& ignoreId) {
	// BUILD REQUEST
	String timeMin = safeMyTZ.dateTime(startTime, UTC_TIME, RFC3339);
	timeMin.replace("+", "%2b");
	// Add milliseconds to ignore events that end in this second
	timeMin = timeMin.substring(0, 19) + ".001" + timeMin.substring(19);

	String timeMax = safeMyTZ.dateTime(endTime, UTC_TIME, RFC3339);
	timeMax.replace("+", "%2b");

	log_i("Sending event isFree request for %s to %s", timeMin.c_str(), timeMax.c_str());

	String url = "https://graph.microsoft.com/v1.0/users/" + _roomEmail
	             + "/calendarView?startDateTime=" + timeMin + "&endDateTime=" + timeMin
	             + "&$select=id&$orderby=start/dateTime&$top=2";

	_http.begin(url);
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);
	_http.addHeader("Prefer", "outlook.timezone=\"UTC\"");

	// SEND REQUEST
	int httpCode = _http.GET();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	log_i("Received event list response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(EVENT_LIST_MAX_SIZE);
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<bool>::makeErr(err);

	JsonArrayConst items = doc["value"].as<JsonArrayConst>();
	for (JsonObjectConst item : items) {
		if (item["id"].as<String>() == ignoreId)
			continue;

		// Is not free, because an accepted event overlaps
		// with timespan between startTime and endTime
		return Result<bool>::makeOk(new bool(false));
	}

	return Result<bool>::makeOk(new bool(true));
}

Result<String> MicrosoftAPI::getRoomName() {  // BUILD REQUEST
	String url = "https://graph.microsoft.com/v1.0/users/" + _roomEmail + "/calendar?$select=owner";

	_http.begin(url);
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// SEND REQUEST
	int httpCode = _http.GET();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	log_i("Received room name response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(NAME_GET_MAX_SIZE);
	auto err = parseJSONResponse(doc, httpCode, responseBody);
	if (err)
		return Result<String>::makeErr(err);

	return Result<String>::makeOk(new String(doc["owner"]["name"].as<String>()));
}

std::shared_ptr<Event> MicrosoftAPI::extractEvent(JsonObjectConst object) {
	auto start = timeutils::parseRfcTimestamp(object["start"]["dateTime"].as<String>() + "Z");
	auto end = timeutils::parseRfcTimestamp(object["end"]["dateTime"].as<String>() + "Z");
	return std::shared_ptr<Event>(new Event{
	    .id = object["id"],
	    // object["organizer"]["emailAddress"]["name"] could also be used
	    .creator = object["organizer"]["emailAddress"]["address"],
	    .summary = object["subject"] | "(No title)",
	    .unixStartTime = start,
	    .unixEndTime = end,
	});
}

}  // namespace cal