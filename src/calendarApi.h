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
	Result(std::shared_ptr<T> _ok, std::shared_ptr<Error> _err) : ok_{_ok}, err_{_err} {}

	const std::shared_ptr<T> ok_ = nullptr;
	const std::shared_ptr<Error> err_ = nullptr;

  public:
	Result(const Result& other) = default;
	Result(Result&& other) = default;
	Result& operator=(const Result& other) = default;
	Result& operator=(Result&& other) = default;
	virtual ~Result() = default;

	static Result makeOk(T* value) {
		if (!value)
			throw std::runtime_error("Tried to make an ok result with a null pointer");
		return Result(std::shared_ptr<T>(value), nullptr);
	}

	static Result makeOk(std::shared_ptr<T> value) {
		if (!value)
			throw std::runtime_error("Tried to make an ok result with a null pointer");
		return Result(value, nullptr);
	}

	static Result makeErr(Error* error) {
		if (!error)
			throw std::runtime_error("Tried to make an error result with a null pointer");
		return Result(nullptr, std::shared_ptr<Error>(error));
	}

	static Result makeErr(std::shared_ptr<T> error) {
		if (!error)
			throw std::runtime_error("Tried to make an error result with a null pointer");
		return Result(nullptr, error);
	}

	bool isOk() const { return ok_ != nullptr; }
	bool isErr() const { return err_ != nullptr; }

	std::shared_ptr<T> ok() const {
		assert(isOk());
		return ok_;
	}

	std::shared_ptr<Error> err() const {
		assert(isErr());
		return err_;
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

/**
 * Get relevant values from json object and create an Events struct from them.
 * Some fields can be empty if the json object doesn't contain them.
 */
std::shared_ptr<Event> extractEvent(const JsonObject& object);

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