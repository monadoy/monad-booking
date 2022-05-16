#include "calendarApi.h"

#include <HTTPClient.h>

#include "cert.h"

namespace calapi {
namespace internal {

const char* EVENT_FIELDS = "id,creator,start,end,summary,attendees(organizer,responseStatus)";

std::shared_ptr<Event> extractEvent(const JsonObject& object) {
	// Whole day events contain "date" key, ignore these for now
	if (object["start"].containsKey("date"))
		return nullptr;

	// Check that this room has accepted the event.
	// It will not be accepted if it would overlap with another event in this room.
	JsonArray attendees = object["attendees"].as<JsonArray>();
	bool roomAccepted = false;
	for (JsonObject attendee : attendees) {
		if ((attendee["organizer"] | false) == true && attendee["responseStatus"] == "accepted") {
			roomAccepted = true;
			break;
		}
	}
	if (!roomAccepted)
		return nullptr;

	// Extract the values from the JSON object

	time_t startTime = parseRfcTimestamp(object["start"]["dateTime"]);
	time_t endTime = parseRfcTimestamp(object["end"]["dateTime"]);

	return std::shared_ptr<Event>(new Event{
	    .id = object["id"],
	    .creator = object["creator"]["displayName"] | object["creator"]["email"],
	    .summary = object["summary"] | "(No title)",
	    .unixStartTime = startTime,
	    .unixEndTime = endTime,
	});
}

void refresh(Token& token) {
	Serial.println("Refreshing token...");
	if (token.unixExpiry + 10 > UTC.now()) {
		Serial.println("Token doesn't need refreshing");
		return;
	}

	HTTPClient http;
	http.begin(token.tokenUri, GOOGLE_API_FULL_CHAIN_CERT);
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	int httpCode
	    = http.POST("client_id=" + token.clientId + "&client_secret=" + token.clientSecret
	                + "&refresh_token=" + token.refreshToken + "&grant_type=refresh_token");

	String responseBody = http.getString();
	Serial.println("Received token response:\n" + responseBody + "\n");

	if (httpCode == 200) {
		StaticJsonDocument<1024> doc;
		DeserializationError err = deserializeJson(doc, responseBody);
		if (err) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(err.f_str());
		}

		token.accessToken = doc["access_token"].as<String>();
		token.unixExpiry = UTC.now() + doc["expires_in"].as<long>();
	} else {
		Serial.println("Error on HTTP request: " + httpCode);
	}

	http.end();
}

time_t getNextMidnight(Timezone& myTZ) {
	time_t now = myTZ.now();

	tmElements_t tm;
	ezt::breakTime(now, tm);
	tm.Hour = 0;
	tm.Minute = 0;
	tm.Second = 0;
	tm.Day += 1;

	time_t nextMidnight = ezt::makeTime(tm);

	return nextMidnight;
}
}  // namespace internal

time_t parseRfcTimestamp(const String& input) {
	tmElements_t tmElements{};
	tmElements.Year = input.substring(0, 4).toInt() - 1970;
	tmElements.Month = input.substring(5, 7).toInt();
	tmElements.Day = input.substring(8, 10).toInt();
	tmElements.Hour = input.substring(11, 13).toInt();
	tmElements.Minute = input.substring(14, 16).toInt();
	tmElements.Second = input.substring(17, 19).toInt();

	time_t t = ezt::makeTime(tmElements);

	// Find position of timezone, because we need to ignore fractions of seconds
	unsigned int timeZonePos = 19;
	if (input.charAt(19) == '.') {
		timeZonePos = input.indexOf("Z", 19);
		if (timeZonePos == -1)
			timeZonePos = input.indexOf("+", 19);
		if (timeZonePos == -1)
			timeZonePos = input.indexOf("-", 19);
	}

	// Timezone handling
	if (input.charAt(timeZonePos) != 'Z') {
		int sign = 1;
		if (input.charAt(timeZonePos) == '-') {
			sign = -1;
		}

		int offsetHours = input.substring(timeZonePos + 1, timeZonePos + 3).toInt();
		int offsetMinutes = input.substring(timeZonePos + 4, timeZonePos + 6).toInt();

		t -= sign * offsetHours * SECS_PER_HOUR;
		t -= sign * offsetMinutes * SECS_PER_MIN;
	}

	return t;
}

Token parseToken(const String& input) {
	StaticJsonDocument<1024> doc;
	DeserializationError err = deserializeJson(doc, input);
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
	}

	return Token{.accessToken = doc["token"],
	             .refreshToken = doc["refresh_token"],
	             .tokenUri = doc["token_uri"],
	             .clientId = doc["client_id"],
	             .clientSecret = doc["client_secret"],
	             .scope = doc["scopes"][0],
	             // Init to zero as we can't parse time strings to unix time.
	             // Needs to be refreshed.
	             .unixExpiry = 0};
}

void printToken(const Token& token) {
	Serial.println("TOKEN: {");
	Serial.println("  Access Token: " + token.accessToken);
	Serial.println("  Refresh Token: " + token.refreshToken);
	Serial.println("  Token URI: " + token.tokenUri);
	Serial.println("  Client ID: " + token.clientId);
	Serial.println("  Client Secret: " + token.clientSecret);
	Serial.println("  Client Scope: " + token.scope);
	Serial.print("  Unix Expiry: ");
	Serial.println(token.unixExpiry);
	Serial.println("  Expiry Timestamp: " + UTC.dateTime(token.unixExpiry, UTC_TIME, RFC3339));
	Serial.println("}");
}

Result<CalendarStatus> fetchCalendarStatus(Token& token, Timezone& myTZ, const String& calendarId) {
	internal::refresh(token);
	HTTPClient http;

	String timeMin = myTZ.dateTime(RFC3339);
	timeMin.replace("+", "%2b");

	String timeMax = myTZ.dateTime(internal::getNextMidnight(myTZ), RFC3339);
	timeMax.replace("+", "%2b");

	String timeZone = myTZ.getOlson();

	String url = "https://www.googleapis.com/calendar/v3/calendars/" + calendarId
	             + "/events?timeMin=" + timeMin + "&timeMax=" + timeMax + "&timeZone=" + timeZone
	             + "&singleEvents=true&orderBy=startTime&fields=summary,items("
	             + internal::EVENT_FIELDS + ")";

	http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);

	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", "Bearer " + token.accessToken);

	int httpCode = http.GET();

	String responseBody = http.getString();
	http.end();

	Serial.println("Received event list response:\n" + responseBody + "\n");

	DynamicJsonDocument doc(2048);
	DeserializationError err = deserializeJson(doc, responseBody);
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		return Result<CalendarStatus>::makeErr(new Error{.code = 0, .message = err.f_str()});
	}

	if (httpCode != 200) {
		Serial.println("Error on HTTP request: " + httpCode);
		return Result<CalendarStatus>::makeErr(
		    new Error{.code = httpCode, .message = doc["error"]["message"]});
	}

	auto status = new CalendarStatus{
	    .name = "",
	    .currentEvent = nullptr,
	    .nextEvent = nullptr,
	};

	status->name = doc["summary"].as<String>();

	JsonArray items = doc["items"].as<JsonArray>();

	time_t now = UTC.now();

	for (JsonObject item : items) {
		std::shared_ptr<Event> event = internal::extractEvent(item);

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
}

void printEvent(const Event& event) {
	Serial.println("EVENT: {");
	Serial.println("  Id: " + event.id);
	Serial.println("  Creator: " + event.creator);
	Serial.println("  Summary: " + event.summary);
	Serial.println("  Start Timestamp: " + UTC.dateTime(event.unixStartTime, UTC_TIME, RFC3339));
	Serial.println("  End Timestamp: " + UTC.dateTime(event.unixEndTime, UTC_TIME, RFC3339));
	Serial.println("}");
}

Result<Event> endEvent(Token& token, Timezone& myTZ, const String& calendarId,
                       const String& eventId) {
	internal::refresh(token);
	HTTPClient http;

	String nowStr = myTZ.dateTime(RFC3339);

	String url = "https://www.googleapis.com/calendar/v3/calendars/" + calendarId + "/events/"
	             + eventId + "?fields=" + internal::EVENT_FIELDS;

	http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);

	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", "Bearer " + token.accessToken);
	StaticJsonDocument<200> payloadDoc;
	payloadDoc["end"] = JsonObject{};
	payloadDoc["end"]["dateTime"] = nowStr;
	payloadDoc["end"]["timeZone"] = myTZ.getOlson();

	String payload = "";
	serializeJson(payloadDoc, payload);

	int httpCode = http.PATCH(payload);

	String responseBody = http.getString();
	http.end();

	Serial.println("Received event patch response:\n" + responseBody + "\n");

	StaticJsonDocument<500> doc;
	DeserializationError err = deserializeJson(doc, responseBody);
	if (err) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(err.f_str());
		return Result<Event>::makeErr(new Error{.code = 0, .message = err.f_str()});
	}

	if (httpCode != 200) {
		Serial.println("Error on HTTP request: " + httpCode);
		return Result<Event>::makeErr(
		    new Error{.code = httpCode, .message = doc["error"]["message"]});
	}

	std::shared_ptr<Event> event = internal::extractEvent(doc.as<JsonObject>());

	if (!event) {
		return Result<Event>::makeErr(
		    new Error{.code = 0, .message = "PATCH didn't return a valid event"});
	}

	return Result<Event>::makeOk(event);
}
}  // namespace calapi