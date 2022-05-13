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
	std::shared_ptr<Event> currentEvent;
	std::shared_ptr<Event> nextEvent;
};

/**
 * Contains either a successful result or an error.
 * Check first which value is contained with functions isOk() or isErr().
 * Then you can get a shared_ptr to the value with functions ok() or err().
 */
template <typename T>
struct Result {
  private:
	Result(std::shared_ptr<T> _ok, std::shared_ptr<Error> _err) {
		_ok = _ok;
		_err = _err;
	};

	std::shared_ptr<T> _ok;
	std::shared_ptr<Error> _err;

  public:
	static Result makeOk(T* value) { return Result(std::shared_ptr<T>(value), nullptr); }

	static Result makeOk(std::shared_ptr<T> value) { return Result(value, nullptr); }

	static Result makeErr(Error* error) { return Result(nullptr, std::shared_ptr<Error>(error)); }

	static Result makeErr(std::shared_ptr<T> error) { return Result(nullptr, error); }

	bool isOk() { return ok != nullptr; }
	bool isErr() { return err != nullptr; }

	std::shared_ptr<T> ok() {
		assert(_ok != nullptr);
		return _ok;
	}

	std::shared_ptr<Error> err() {
		assert(_err != nullptr);
		return _err;
	}
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

Result<Event> endEvent(Token& token, Timezone& myTZ, const String& calendarId,
                       const String& eventId);

void printEvent(const Event& event);

}  // namespace calapi

#endif