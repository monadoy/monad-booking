#ifndef GOOGLE_API_H
#define GOOGLE_API_H

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "SafeTimezone.h"
#include "api.h"
#include "utils.h"

namespace {

std::shared_ptr<cal::Error> deserializeResponse(JsonDocument& doc, int httpCode,
                                                const String& responseBody);

std::shared_ptr<cal::Event> extractEvent(const JsonObject& object);

}  // namespace

namespace cal {

class GoogleAPI : public API {
  public:
	GoogleAPI(const Token& token, SafeTimezone& tz, SafeTimezone& utc, const String& calendarId);

	bool refreshAuth() override final;
	Result<CalendarStatus> fetchCalendarStatus() override final;

	static utils::Result<Token, utils::Error> parseToken(const JsonObjectConst& obj);

  private:
	Token _token;
	SafeTimezone& _tz;
	SafeTimezone& _utc;
	String _calendarId;

	HTTPClient _http;
};

}  // namespace cal

#endif