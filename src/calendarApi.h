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

struct Error {
	int code;
	String message;
};

struct CalendarStatus {
	String name;
	std::unique_ptr<Event> currentEvent;
	std::unique_ptr<Event> nextEvent;
};

template <typename T>
struct Result {
	static Result makeOk(T* value) {
		return Result{
		    .ok = std::unique_ptr<T>(value),
		    .err = nullptr,
		};
	}

	static Result makeOk(std::unique_ptr<T> value) {
		return Result{
		    .ok = std::move(value),
		    .err = nullptr,
		};
	}

	static Result makeErr(Error* error) {
		return Result{
		    .ok = nullptr,
		    .err = std::unique_ptr<Error>(error),
		};
	}

	static Result makeErr(std::unique_ptr<T> error) {
		return Result{
		    .ok = nullptr,
		    .err = std::move(error),
		};
	}

	bool isOk() { return ok != nullptr; }
	bool isErr() { return err != nullptr; }

	std::unique_ptr<T> ok;
	std::unique_ptr<Error> err;
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

std::unique_ptr<Event> extractEvent(const JsonObject& object);

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
Result<CalendarStatus> fetchCalendarStatus(Token& token, Timezone& myTZ, const String& calendarId);


void printEvent(const Event& event);

}  // namespace calapi

#endif