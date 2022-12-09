#include "microsoftApi.h"

#include "cert.h"
#include "globals.h"
#include "timeUtils.h"
#include "utils.h"

namespace cal {

MicrosoftAPI::MicrosoftAPI(const Token& token, const String& calendarId)
    : _token{token}, _calendarId{calendarId} {
	_http.setReuse(true);
};

const int AUTH_RESPONSE_MAX_SIZE = 4096;

bool MicrosoftAPI::refreshAuth() {
	log_i("Refreshing token...");
	if (_token.unixExpiry + 60 > safeUTC.now()) {
		log_i("Token doesn't need refreshing");
		return true;
	}

	// BUILD REQUEST
	_http.begin("https://login.microsoftonline.com/organizations/oauth2/v2.0/token",
	            MICROSOFT_LOGIN_API_FULL_CHAIN_CERT);
	_http.addHeader("Content-Type", "application/x-www-form-urlencoded");

	// SEND REQUEST
	int httpCode = _http.POST("client_id=" + _token.clientId + "&refresh_token="
	                          + _token.refreshToken + "&grant_type=refresh_token");

	// PARSE RESPONSE AS JSON
	String responseBody = _http.getString();
	_http.end();
	log_i("Received refresh auth response:\n%s", responseBody.c_str());
	DynamicJsonDocument doc(AUTH_RESPONSE_MAX_SIZE);
	auto err = deserializeResponse(doc, httpCode, responseBody);
	if (err)
		return false;

	// PARSE JSON VALUES INTO TOKEN
	_token.accessToken = doc["access_token"].as<String>();
	_token.unixExpiry = safeUTC.now() + doc["expires_in"].as<long>();
	if (doc.containsKey("refresh_token")) {
		_token.refreshToken = doc["refresh_token"].as<String>();
	}

	assert(_saveTokenFunc);
	_saveTokenFunc(_token);

	return true;
};

Result<CalendarStatus> MicrosoftAPI::fetchCalendarStatus() {
	// UNIMPLEMENTED
	assert(false);
	return Result<CalendarStatus>::makeErr(new Error{Error::Type::LOGICAL, "Unimplemented"});
};

Result<Event> MicrosoftAPI::endEvent(const String& eventId) {
	// UNIMPLEMENTED
	assert(false);
	return Result<Event>::makeErr(new Error{Error::Type::LOGICAL, "Unimplemented"});
}

Result<Event> MicrosoftAPI::insertEvent(time_t startTime, time_t endTime) {
	// UNIMPLEMENTED
	assert(false);
	return Result<Event>::makeErr(new Error{Error::Type::LOGICAL, "Unimplemented"});
}

Result<Event> MicrosoftAPI::rescheduleEvent(std::shared_ptr<Event> event, time_t newStartTime,
                                            time_t newEndTime) {
	// UNIMPLEMENTED
	assert(false);
	return Result<Event>::makeErr(new Error{Error::Type::LOGICAL, "Unimplemented"});
}

std::shared_ptr<cal::Error> MicrosoftAPI::deserializeResponse(JsonDocument& doc, int httpCode,
                                                              const String& responseBody) {
	// UNIMPLEMENTED
	assert(false);
	return std::make_shared<Error>(Error::Type::LOGICAL, "Unimplemented");
}

bool MicrosoftAPI::isRoomAccepted(JsonObjectConst eventObject) {
	// UNIMPLEMENTED
	assert(false);
	return false;
}

Result<bool> MicrosoftAPI::isFree(time_t startTime, time_t endTime, const String& ignoreId) {
	// UNIMPLEMENTED
	assert(false);
	return Result<bool>::makeErr(new Error{Error::Type::LOGICAL, "Unimplemented"});
}

std::shared_ptr<Event> MicrosoftAPI::extractEvent(JsonObjectConst object, bool ignoreRoomAccept) {
	// UNIMPLEMENTED
	// UNIMPLEMENTED
	assert(false);
	return nullptr;
}

}  // namespace cal