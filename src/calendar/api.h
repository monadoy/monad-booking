#ifndef API_H
#define API_H

#include <Arduino.h>

#include <memory>

#include "utils.h"

namespace cal {

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
	Error(const String& message) : message(message), code(0) {}
	Error(int code, const String& message) : message(message), code(code) {}
	int code;
	String message;
};

struct CalendarStatus {
	String name;
	std::shared_ptr<Event> currentEvent;
	std::shared_ptr<Event> nextEvent;
};

template <typename T>
using Result = utils::Result<T, Error>;

/*
Interface for working with a calendar API endpoint.
All time_t variables are unix epoch timestamps in UTC.
Single-threaded by design!!
Internal state is not mutex protected!!
*/
class API {
  public:
	virtual ~API() = default;

	/*
	 * Fetch new access tokens from API to ensure that other
	 * API functions can be called.
	 * Should be called every time before calling other functions.
	 * Should only refresh if token is actually close to expiration.
	 */
	virtual bool refreshAuth() = 0;

	/**
	 * Fetch the current and next events from calendar today.
	 * Also contains the name of the room.
	 * Returns the calendar status on success and error on failure.
	 */
	virtual Result<CalendarStatus> fetchCalendarStatus() = 0;

	/**
	 * Request calendar to end the event pointed by eventId.
	 * newEndTime should be lower than the current end time.
	 * Returns the changed event on success and error on failure.
	 */
	virtual Result<Event> endEvent(const String& eventId) = 0;

	/**
	 * Insert an event to the calendar.
	 * Fails if an event already exists between startTime and endTime.
	 * Returns the new event on success and error on failure.
	 */
	virtual Result<Event> insertEvent(time_t startTime, time_t endTime) = 0;
};

}  // namespace cal
#endif