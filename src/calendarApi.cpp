#include "calendarApi.h"

#include <HTTPClient.h>

#include "cert.h"

namespace calapi {
namespace internal {
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

CalendarStatus fetchCalendarStatus(Token& token, Timezone& myTZ, const String& calendarId) {
	internal::refresh(token);
	HTTPClient http;

	String timeMin = myTZ.dateTime(RFC3339);
	timeMin.replace("+", "%2b");

	String timeMax = myTZ.dateTime(internal::getNextMidnight(myTZ), RFC3339);
	timeMax.replace("+", "%2b");

	String timeZone = myTZ.getOlson();

	String url = "https://www.googleapis.com/calendar/v3/calendars/" + calendarId + 
		"/events?timeMin=" + timeMin + 
		"&timeMax=" + timeMax + 
		"&timeZone=" + timeZone + 
		"&singleEvents=true&orderBy=startTime&maxResults=2&fields=summary,items(id,creator,start,end,summary)";

	http.begin(url, GOOGLE_API_FULL_CHAIN_CERT);

	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", "Bearer " + token.accessToken);

	int httpCode = http.GET();

	CalendarStatus result{
	    .name = "",
	    .currentEvent = nullptr,
	    .nextEvent = nullptr,
	};

	String responseBody = http.getString();
	Serial.println("Received event list response:\n" + responseBody + "\n");

	if (httpCode == 200) {
		DynamicJsonDocument doc(2048);
		DeserializationError err = deserializeJson(doc, responseBody);
		if (err) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(err.f_str());
		}

		result.name = doc["summary"].as<String>();

		JsonArray items = doc["items"].as<JsonArray>();

		time_t now = UTC.now();

		for (JsonObject item : items) {
			// Whole day events contain "date" key, ignore these for now
			if (item["start"].containsKey("date"))
				continue;

			time_t startTime = parseRfcTimestamp(item["start"]["dateTime"]);
			time_t endTime = parseRfcTimestamp(item["end"]["dateTime"]);

			auto createEvent = [&](JsonObject& o) {
				return new Event{
				    .id = o["id"],
				    .creator = o["creator"]["displayName"] | o["creator"]["email"],
				    .summary = o["summary"],
				    .unixStartTime = startTime,
				    .unixEndTime = endTime,
				};
			};

			if (startTime <= now && now <= endTime && result.currentEvent == nullptr) {
				result.currentEvent = std::unique_ptr<Event>(createEvent(item));
			} else if (startTime > now && result.nextEvent == nullptr) {
				result.nextEvent = std::unique_ptr<Event>(createEvent(item));
			}
		}
	} else {
		Serial.println("Error on HTTP request: " + httpCode);
	}

	http.end();

	return result;
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
}  // namespace calapi