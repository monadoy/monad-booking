#ifndef API_H
#define API_H

#include <Arduino.h>

#include <memory>

#include "utils.h"

namespace cal {

struct Token {
	String accessToken;
	String refreshToken;
	String clientId;
	String clientSecret;
	String scope;
	time_t unixExpiry;

	void print() const {
		Serial.println("Token:");
		Serial.println("  access_token: " + accessToken);
		Serial.println("  refresh_token: " + refreshToken);
		Serial.println("  client_id: " + clientId);
		Serial.println("  client_secret: " + clientSecret);
		Serial.println("  scope: " + scope);
		Serial.println("  expiry: " + String(unixExpiry));
	}
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
	enum class Type {
		// In HTTP errors the web requests have returned a non-ok status code.
		// E.g. internal server errors, invalid requests, forbidden requests.
		HTTP,
		// Parse errors are e.g. mangled json objects, empty responses,
		// or non existing json keys.
		PARSE,
		// In logical errors we have otherwise valid web responses, but some invariants are broken.
		// E.g. inserting overlapping events, or removing non-existing events.
		LOGICAL
	};
	Error(Type type, const String& message) : type{type}, message{message} {}
	Type type;
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

	virtual void registerSaveTokenFunc(std::function<void(const Token&)> saveTokenFunc) = 0;

	/**
	 * Fetch the current and next events from calendar today. Also contains the name
	 * of the room. Returns the calendar status on success and error on failure.
	 */
	virtual Result<CalendarStatus> fetchCalendarStatus() = 0;

	/**
	 * Request calendar to end the event pointed by eventId.
	 * newEndTime must be lower than the current end time.
	 * Returns the changed event on success and error on failure.
	 */
	virtual Result<Event> endEvent(const String& eventId) = 0;

	/**
	 * Insert an event to the calendar.
	 * Fails if an event already exists between startTime and endTime.
	 * Returns the new event on success and error on failure.
	 */
	virtual Result<Event> insertEvent(time_t startTime, time_t endTime) = 0;

	/**
	 * Extend an event in the calendar.
	 * Fails if an event already exists between the newStartTime and newEndTime.
	 * Returns the new event on success and error on failure.
	 */
	virtual Result<Event> rescheduleEvent(std::shared_ptr<Event> event, time_t newStartTime,
	                                      time_t newEndTime)
	    = 0;
};

// Helpers:
utils::Result<Token, utils::Error> jsonToToken(JsonObjectConst obj);
void tokenToJson(JsonObject& resultObj, const Token& token);

std::shared_ptr<cal::Error> parseJSONResponse(JsonDocument& doc, int httpCode,
                                              const String& responseBody);

}  // namespace cal
#endif