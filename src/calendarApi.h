#ifndef CALENDAR_API_H
#define CALENDAR_API_H

#include <ArduinoJson.h>

namespace calapi {

struct Token {
	String accessToken;
	String refreshToken;
	String tokenUri;
	String clientId;
	String clientSecret;
	String scope;
	time_t unixExpiry;
};

/**
 * Parses an RFC 3339 time string (e.g. 2022-01-01T00:00:00+03:00) into a UTC time_t,
 */
time_t parseRfcTimestamp(const String& input);

/**
 * Parses a token json into a Token struct.
 */
Token parseToken(Stream& input);

void printToken(Token& token);

/**
 * Request new token from google if token has expired.
 * Updates the token parameter new values.
 * Does nothing if token not expired.
 */
void refresh(Token& token);
}  // namespace calapi

#endif