#include "googleApi.h"

#include "cert.h"
#include "globals.h"
#include "timeUtils.h"

namespace {

std::shared_ptr<cal::Event> extractEvent(const JsonObject& object) {
	// Whole day events contain "date" key, ignore these for now
	if (object["start"].containsKey("date"))
		return nullptr;

	// Check that this room has accepted the event.
	// It will not be accepted if it would overlap with another event in this room.
	JsonArray attendees = object["attendees"].as<JsonArray>();
	bool roomAccepted = false;
	for (JsonObject attendee : attendees) {
		if ((attendee["resource"] | false) == true && attendee["responseStatus"] == "accepted") {
			roomAccepted = true;
			break;
		}
	}
	if (!roomAccepted)
		return nullptr;

	// Extract the values from the JSON object

	time_t startTime = timeutils::parseRfcTimestamp(object["start"]["dateTime"]);
	time_t endTime = timeutils::parseRfcTimestamp(object["end"]["dateTime"]);

	return std::shared_ptr<cal::Event>(new cal::Event{
	    .id = object["id"],
	    .creator = object["creator"]["displayName"] | object["creator"]["email"],
	    .summary = object["summary"] | "(No title)",
	    .unixStartTime = startTime,
	    .unixEndTime = endTime,
	});
}
}  // namespace

namespace cal {

const char* EVENT_FIELDS = "id,creator,start,end,summary,attendees(resource,responseStatus)";
const char* NEW_EVENT_SUMMARY = "M5Paper Event";

// Techincally we need to fetch only two events to gain knowledge of the current and next event.
// The problem is that the fetch returns declined events. Fetching too many events will make us run
// out of memory, so we need a some kind of sensible limit. Here we hope the at least two of these
// events are accepted.
const int LIST_MAX_EVENTS = 16;

// A single event takes around 600 bytes plus 50 per each additional attendee.
// We limit the attendee count to one, because the room seems to be listed first, and it's the only
// one we need to check.
const int EVENT_MAX_SIZE = 1024;

const int EVENT_LIST_MAX_SIZE = LIST_MAX_EVENTS * EVENT_MAX_SIZE;

GoogleAPI::GoogleAPI(const Token& token, const String& calendarId)
    : _token{token}, _calendarId{calendarId} {};

bool GoogleAPI::refreshAuth() {
	Serial.println("Refreshing token...");
	if (_token.unixExpiry + 60 > safeUTC.now()) {
		Serial.println("Token doesn't need refreshing");
		return true;
	}

	// BUILD REQUEST
	_http.begin(_token.tokenUri, GOOGLE_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	// SEND REQUEST
	int httpCode
	    = _http.POST("client_id=" + _token.clientId + "&client_secret=" + _token.clientSecret
	                 + "&refresh_token=" + _token.refreshToken + "&grant_type=refresh_token");

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	Serial.println("Received token response:\n" + responseBody + "\n");
	if (httpCode != 200) {
		Serial.println("Error on HTTP request: " + httpCode);
		return false;
	}
	StaticJsonDocument<1024> doc;
	DeserializationError err = deserializeJson(doc, responseBody);
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		return false;
	}

	// PARSE JSON VALUES INTO TOKEN
	_token.accessToken = doc["access_token"].as<String>();
	_token.unixExpiry = safeUTC.now() + doc["expires_in"].as<long>();

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

	_http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// SEND REQUEST
	int httpCode = _http.GET();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	Serial.println("Received event list response:\n" + responseBody + "\n");
	DynamicJsonDocument doc(EVENT_LIST_MAX_SIZE);
	auto err = deserializeResponse(doc, httpCode, responseBody);
	if (err)
		return Result<CalendarStatus>::makeErr(err);

	// PARSE JSON AS CALENDAR STATUS STRUCT
	auto status = new CalendarStatus{
	    .name = "",
	    .currentEvent = nullptr,
	    .nextEvent = nullptr,
	};

	status->name = doc["summary"].as<String>();
	JsonArray items = doc["items"].as<JsonArray>();
	now = safeUTC.now();

	for (JsonObject item : items) {
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
	_http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// CREATE PAYLOAD
	StaticJsonDocument<256> payloadDoc;
	payloadDoc["end"] = JsonObject{};
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

	Serial.println("Received event patch response:\n" + responseBody + "\n");
	StaticJsonDocument<1024> doc;
	auto err = deserializeResponse(doc, httpCode, responseBody);
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
	// BUILD REQUEST
	String url
	    = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId + "/events?fields=id";
	_http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// CREATE PAYLOAD
	StaticJsonDocument<256> payloadDoc;
	payloadDoc["start"] = JsonObject{};
	payloadDoc["start"]["dateTime"] = safeMyTZ.dateTime(startTime, UTC_TIME, RFC3339);
	payloadDoc["start"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["end"] = JsonObject{};
	payloadDoc["end"]["dateTime"] = safeMyTZ.dateTime(endTime, UTC_TIME, RFC3339);
	payloadDoc["end"]["timeZone"] = safeMyTZ.getOlson();
	payloadDoc["summary"] = NEW_EVENT_SUMMARY;
	String payload = "";
	serializeJson(payloadDoc, payload);

	Serial.println("Sending event insert payload:\n" + payload + "\n");

	// SEND REQUEST
	int httpCode = _http.POST(payload);
	payload.clear();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();

	Serial.println("Received event insert response:\n" + responseBody + "\n");
	DynamicJsonDocument doc(1024);
	auto err = deserializeResponse(doc, httpCode, responseBody);
	if (err)
		return Result<Event>::makeErr(err);

	// PARSE JSON TO GET EVENT ID
	String eventId = doc["id"];

	// We need to fetch the room to check if the room has accepted it.
	// It will be declined if it overlaps with an existing events.

	// Google may not instantly update if room accepts or not, so try a few times.
	int i = 0;
	auto res = getEvent(eventId);

	while (i < INSERT_EVENT_VERIFY_MAX_RETRIES && res.isErr()
	       && res.err()->type == Error::Type::LOGICAL) {
		res = getEvent(eventId);
		i++;
	}

	return res;
}

Result<Event> GoogleAPI::getEvent(const String& eventId) {
	// BUILD REQUEST
	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId + "/events/"
	             + eventId + "?maxAttendees=1&fields=" + EVENT_FIELDS;
	_http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	// SEND REQUEST
	int httpCode = _http.GET();

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();

	Serial.println("Received event get response:\n" + responseBody + "\n");
	DynamicJsonDocument doc(1024);
	auto err = deserializeResponse(doc, httpCode, responseBody);
	if (err)
		return Result<Event>::makeErr(err);

	// PARSE JSON AS EVENT STRUCT
	std::shared_ptr<Event> event = extractEvent(doc.as<JsonObject>());
	if (!event) {
		return Result<Event>::makeErr(
		    new Error{Error::Type::LOGICAL, "GET didn't return a valid accepted event"});
	}

	return Result<Event>::makeOk(event);
}

utils::Result<Token, utils::Error> GoogleAPI::parseToken(const JsonObjectConst& obj) {
	if (!obj) {
		return utils::Result<Token, utils::Error>::makeErr(
		    new utils::Error{"Token parse failed, token is null"});
	}

	if (!(obj["token"] && obj["refresh_token"] && obj["token_uri"] && obj["client_id"]
	      && obj["client_secret"] && obj["scopes"][0])) {
		return utils::Result<Token, utils::Error>::makeErr(
		    new utils::Error{"Token parse failed, missing some required keys"});
	}

	return utils::Result<Token, utils::Error>::makeOk(
	    new Token{.accessToken = obj["token"],
	              .refreshToken = obj["refresh_token"],
	              .tokenUri = obj["token_uri"],
	              .clientId = obj["client_id"],
	              .clientSecret = obj["client_secret"],
	              .scope = obj["scopes"][0],
	              .unixExpiry = 0});
}

std::shared_ptr<cal::Error> GoogleAPI::deserializeResponse(JsonDocument& doc, int httpCode,
                                                           const String& responseBody) {
	// Negative codes are errors special to HTTPClient
	if (httpCode < 0) {
		String errStr = _http.errorToString(httpCode);
		log_w("HTTP: %d, %s", httpCode, errStr.c_str());
		return std::make_shared<cal::Error>(cal::Error::Type::HTTP,
		                                    "HTTP: " + String(httpCode) + ", " + errStr.c_str());
	}

	DeserializationError err = deserializeJson(doc, responseBody);
	if (err) {
		String errStr = err.f_str();
		log_w("deserializeJson() failed with code %s", errStr.c_str());
		return std::make_shared<cal::Error>(cal::Error::Type::PARSE,
		                                    "deserializeJson() failed with code " + errStr);
	}

	if (httpCode < 200 || httpCode >= 300) {
		log_w("HTTP response: %d, %s", httpCode, doc["error"]["message"].as<char*>());
		return std::make_shared<cal::Error>(
		    cal::Error::Type::HTTP,
		    "HTTP response: " + String(httpCode) + ", " + doc["error"]["message"].as<String>());
	}

	return nullptr;
}

}  // namespace cal