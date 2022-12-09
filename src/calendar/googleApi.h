#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "api.h"
#include "utils.h"

namespace cal {

class GoogleAPI : public API {
  public:
	GoogleAPI(const Token& token, const String& calendarId);

	bool refreshAuth() override final;
	Result<CalendarStatus> fetchCalendarStatus() override final;
	Result<Event> endEvent(const String& eventId) override final;
	Result<Event> insertEvent(time_t startTime, time_t endTime) override final;
	Result<Event> rescheduleEvent(std::shared_ptr<Event> event, time_t newStartTime,
	                              time_t newEndTime) override final;

	void registerSaveTokenFunc(std::function<void(const Token&)> saveTokenFunc) override final {
		_saveTokenFunc = saveTokenFunc;
	}

  private:
	/**
	 * Performs some validation on the JSON object and
	 * returns the event as a struct or error on failure.
	 * ignoreRoomAccept can be used when extracting events that have just been created,
	 * as they may not yet be accepted, but we still want to parse them.
	 */
	std::shared_ptr<cal::Event> extractEvent(JsonObjectConst object, bool ignoreRoomAccept = false);

	Result<bool> isFree(time_t startTime, time_t endTime, const String& ignoreId = "");

	bool isRoomAccepted(JsonObjectConst eventObject);

	std::function<void(const Token&)> _saveTokenFunc;

	Token _token;
	String _calendarId;

	HTTPClient _http;
};

}  // namespace cal

#endif