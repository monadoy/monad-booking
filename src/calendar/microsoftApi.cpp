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

bool MicrosoftAPI::refreshAuth() {
	// UNIMPLEMENTED
	assert(false);
	return false;
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

utils::Result<Token, utils::Error> MicrosoftAPI::parseToken(JsonObjectConst obj) {
	// UNIMPLEMENTED
	assert(false);
	return utils::Result<Token, utils::Error>::makeErr(new utils::Error{"Unimplemented"});
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