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

class API {
  public:
	virtual ~API() = default;
	virtual void refreshAuth() = 0;
	virtual Result<CalendarStatus> fetchCalendarStatus() = 0;
};

}  // namespace cal
#endif