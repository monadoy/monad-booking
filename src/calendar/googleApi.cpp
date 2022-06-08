#include "googleApi.h"

namespace cal {

GoogleAPI::GoogleAPI(const Token& token, SafeTimezone& tz, SafeTimezone& utc,
                     const String& calendarId)
    : _token{token}, _tz{tz}, _utc{utc}, _calendarId{calendarId} {};

void GoogleAPI::refreshAuth() { assert(false); };
Result<CalendarStatus> GoogleAPI::fetchCalendarStatus() { assert(false); };

utils::Result<Token, utils::Error> parseToken(const JsonObjectConst& obj) {
	if (!obj) {
		return utils::Result<Token, utils::Error>::makeErr(
		    new utils::Error{"Token parse failed, token is null"});
	}

	if (!(obj["token"] && obj["refresh_token"] && obj["token_uri"] && obj["client_id"]
	      && obj["client_secret"] && obj["scopes"][0])) {
		return utils::Result<Token, utils::Error>::makeErr(
		    new utils::Error{"Token parse failed, missing some required keys"});
	}

	return utils::Result<Token, utils::Error>::makeOk(
	    new Token{.accessToken = obj["token"],
	              .refreshToken = obj["refresh_token"],
	              .tokenUri = obj["token_uri"],
	              .clientId = obj["client_id"],
	              .clientSecret = obj["client_secret"],
	              .scope = obj["scopes"][0],
	              .unixExpiry = 0});
}

}  // namespace cal