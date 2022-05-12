#ifndef CALENDAR_API_H
#define CALENDAR_API_H

#include <ArduinoJson.h>

#include <memory>

#define EZTIME_EZT_NAMESPACE 1
#include <ezTime.h>

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

struct Event {
	String id;
	String creator;
	String summary;
	time_t unixStartTime;
	time_t unixEndTime;
};

struct CalendarStatus {
	String name;
	std::unique_ptr<Event> currentEvent;
	std::unique_ptr<Event> nextEvent;
};

namespace internal {
/**
 * Request new token from google if token has expired.
 * Updates the token parameter new values.
 * Does nothing if token not expired.
 */
void refresh(Token& token);

/**
 * Get LOCAL time_t representing today's midnight in timezone provided by myTZ.
 */
time_t getNextMidnight(Timezone& myTZ);

}  // namespace internal

/**
 * Parses an RFC 3339 time string into a UTC time_t,
 * Not a full RFC 3339 parser, but common (and used by google) formats shown below are supported.
 * 2022-12-30T23:59:59Z
 * 2022-12-30T23:59:59+03:00
 * 2022-12-30T23:59:59-03:00
 * 2022-12-30T23:59:59.000Z
 * 2022-12-30T23:59:59.000+03:00
 * 2022-12-30T23:59:59.000-03:00
 * Seconds may contain any number of digits after the decimal point.
 */
time_t parseRfcTimestamp(const String& input);

/**
 * Parses a token json into a Token struct.
 */
Token parseToken(const String& input);

void printToken(const Token& token);

/**
 * Fetch the current and next events from Google Calendar API.
 * Also contains the name of the room.
 * Refreshes token if necessary.
 * CurrentEvent and/or nextEvent may be nullptr, check before using.
 */
CalendarStatus fetchCalendarStatus(Token& token, Timezone& myTZ, const String& calendarId);

void printEvent(const Event& event);

}  // namespace calapi

#endif