#ifndef CALENDAR_API_H
#define CALENDAR_API_H

#include <ArduinoJson.h>
#include <ezTime.h>

#include <memory>

#include "utils.h"

/**
 * See calendarApi_example.cpp for example usage
 */
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

	bool operator==(const Event& other) {
		return id == other.id && creator == other.creator && summary == other.summary
		       && unixStartTime == other.unixStartTime && unixEndTime == other.unixEndTime;
	};
};

struct Error {
	int code;
	String message;
};

struct CalendarStatus {
	String name;
	std::shared_ptr<Event> currentEvent;
	std::shared_ptr<Event> nextEvent;
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

/**
 * Get relevant values from json object and create an Events struct from them.
 * Some fields can be empty if the json object doesn't contain them.
 */
std::shared_ptr<Event> extractEvent(const JsonObject& object);

std::shared_ptr<Error> deserializeResponse(JsonDocument& doc, int httpCode,
                                           const String& responseBody);

}  // namespace internal

template <typename T>
using Result = utils::Result<T, Error>;

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
 * Returns the calendar status on success and error on failure.
 */
Result<CalendarStatus> fetchCalendarStatus(Token& token, Timezone& myTZ, const String& calendarId);

/**
 * Request Google Calendar API to end the event pointed by calendarId and eventId.
 * Refreshes token if necessary.
 * Returns the changed event on success and error on failure.
 */
Result<Event> endEvent(Token& token, Timezone& myTZ, const String& calendarId,
                       const String& eventId);

/**
 * Insert an event to the calendar using Google Calendar API.
 * startTime and endTIme are unix epoch UTC timestamps.
 * Fails if an event already exists between startTime and endTime.
 * Refreshes token if necessary.
 * Returns the new event on success and error on failure.
 */
Result<Event> insertEvent(Token& token, Timezone& myTZ, const String& calendarId, time_t startTime,
                          time_t endTime);

/**
 * Get an event from the Google Calendar API.
 * Fails if the event is not found or the event is declined.
 * Refreshes token if necessary.
 * Returns the event on success and error on failure.
 */
Result<Event> getEvent(Token& token, const String& calendarId, const String& eventId);

void printEvent(const Event& event);

}  // namespace calapi

#endif