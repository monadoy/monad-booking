#include "googleApi.h"

#include "cert.h"
#include "timeutils.h"

namespace {
std::shared_ptr<cal::Error> deserializeResponse(JsonDocument& doc, int httpCode,
                                                const String& responseBody) {
	DeserializationError err = deserializeJson(doc, responseBody);
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		return std::shared_ptr<cal::Error>(new cal::Error{
		    .code = 0, .message = "deserializeJson() failed with code " + String(err.f_str())});
	}

	if (httpCode != 200) {
		Serial.println("Error in HTTP request: " + httpCode);
		return std::shared_ptr<cal::Error>(
		    new cal::Error{.code = httpCode,
		                   .message = "Error in HTTP request: " + String(httpCode) + ", "
		                              + doc["error"]["message"].as<String>()});
	}

	return nullptr;
}

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

GoogleAPI::GoogleAPI(const Token& token, SafeTimezone& tz, SafeTimezone& utc,
                     const String& calendarId)
    : _token{token}, _tz{tz}, _utc{utc}, _calendarId{calendarId} {};

bool GoogleAPI::refreshAuth() {
	Serial.println("Refreshing token...");
	if (_token.unixExpiry + 60 > _utc.now()) {
		Serial.println("Token doesn't need refreshing");
		return true;
	}

	_http.begin(_token.tokenUri, GOOGLE_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	int httpCode
	    = _http.POST("client_id=" + _token.clientId + "&client_secret=" + _token.clientSecret
	                 + "&refresh_token=" + _token.refreshToken + "&grant_type=refresh_token");

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

	_token.accessToken = doc["access_token"].as<String>();
	_token.unixExpiry = _utc.now() + doc["expires_in"].as<long>();

	return true;
};

Result<CalendarStatus> GoogleAPI::fetchCalendarStatus() {
	time_t now = _tz.now();
	String timeMin = _tz.dateTime(now, RFC3339);
	timeMin.replace("+", "%2b");

	String timeMax = _tz.dateTime(timeutils::getNextMidnight(now), RFC3339);
	timeMax.replace("+", "%2b");

	String timeZone = _tz.getOlson();
	String url = "https://www.googleapis.com/calendar/v3/calendars/" + _calendarId
	             + "/events?timeMin=" + timeMin + "&timeMax=" + timeMax + "&timeZone=" + timeZone
	             + "&maxResults=" + LIST_MAX_EVENTS
	             + "&maxAttendees=1&singleEvents=true&orderBy=startTime&fields=summary,items("
	             + EVENT_FIELDS + ")";

	_http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);

	_http.addHeader("Content-Type", "application/json");
	_http.addHeader("Authorization", "Bearer " + _token.accessToken);

	int httpCode = _http.GET();

	String responseBody = _http.getString();
	_http.end();

	Serial.println("Received event list response:\n" + responseBody + "\n");

	DynamicJsonDocument doc(EVENT_LIST_MAX_SIZE);
	auto err = deserializeResponse(doc, httpCode, responseBody);
	if (err)
		return Result<CalendarStatus>::makeErr(err);

	auto status = new CalendarStatus{
	    .name = "",
	    .currentEvent = nullptr,
	    .nextEvent = nullptr,
	};

	status->name = doc["summary"].as<String>();
	JsonArray items = doc["items"].as<JsonArray>();
	time_t now = _utc.now();

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

utils::Result<Token, utils::Error> parseToken(const JsonObjectConst& obj) {
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

}  // namespace cal