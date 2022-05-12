#include "calendarApi.h"

#include <HTTPClient.h>
#include <StreamUtils.h>
#define EZTIME_EZT_NAMESPACE 1
#include <ezTime.h>

namespace calapi {
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

Token parseToken(Stream& input) {
	ReadLoggingStream loggingStream(input, Serial);
	StaticJsonDocument<1024> doc;
	DeserializationError err = deserializeJson(doc, loggingStream);
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

void refresh(Token& token) {
	Serial.println("Refreshing token...");
	if (token.unixExpiry + 10 > UTC.now()) {
		Serial.println("Token doesn't need refreshing");
		return;
	}

	HTTPClient http;
	http.begin(token.tokenUri);
	http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	int httpCode
	    = http.POST("client_id=" + token.clientId + "&client_secret=" + token.clientSecret
	                + "&refresh_token=" + token.refreshToken + "&grant_type=refresh_token");

	if (httpCode == 200) {
		ReadLoggingStream loggingStream(http.getStream(), Serial);
		StaticJsonDocument<1024> doc;
		DeserializationError err = deserializeJson(doc, loggingStream);
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
}  // namespace calapi