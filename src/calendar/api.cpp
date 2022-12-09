#include "api.h"

#include <HTTPClient.h>

#include "globals.h"
#include "timeUtils.h"

namespace cal {
utils::Result<Token, utils::Error> jsonToToken(JsonObjectConst obj) {
	using TokenRes = utils::Result<Token, utils::Error>;
	if (!obj) {
		return TokenRes::makeErr(new utils::Error{"Token parse failed, token is null."});
	}

	// token is legacy token formats, replaced by access_token
	const auto accessToken = obj["access_token"] | obj["token"];
	const auto refreshToken = obj["refresh_token"];
	const auto clientId = obj["client_id"];
	const auto clientSecret = obj["client_secret"];
	// scopes[0] is for legacy token formats, replaced by scope
	const auto scope = obj["scope"] | obj["scopes"][0];
	const auto expiry = obj["expiry"];

	// Secret is allowed to be null, so we don't check for it. E.g. microsoft doesn't use a secret.
	if (!(accessToken && refreshToken && clientId && /*clientSecret && */ scope && expiry)) {
		return TokenRes::makeErr(
		    new utils::Error{"Token parse failed, missing some required keys."});
	}

	auto token = new Token{.accessToken = accessToken,
	                       .refreshToken = refreshToken,
	                       .clientId = clientId,
	                       .clientSecret = clientSecret,
	                       .scope = scope,
	                       .unixExpiry = timeutils::parseRfcTimestamp(expiry)};

	return TokenRes::makeOk(token);
}

void tokenToJson(JsonObject& resultObj, const Token& token) {
	resultObj["access_token"] = token.accessToken;
	resultObj["refresh_token"] = token.refreshToken;
	resultObj["client_id"] = token.clientId;
	resultObj["client_secret"] = token.clientSecret;
	resultObj["scope"] = token.scope;
	resultObj["expiry"] = safeUTC.dateTime(token.unixExpiry, RFC3339);
}

std::shared_ptr<cal::Error> parseJSONResponse(JsonDocument& doc, int httpCode,
                                              const String& responseBody) {
	// Negative codes are errors special to HTTPClient
	if (httpCode < 0) {
		String errStr = HTTPClient::errorToString(httpCode);
		log_w("HTTP: %d, %s", httpCode, errStr.c_str());
		return std::make_shared<cal::Error>(cal::Error::Type::HTTP,
		                                    "HTTP: " + String(httpCode) + ", " + errStr.c_str());
	}

	if (httpCode < 200 || httpCode >= 300) {
		String errStr = utils::httpCodeToString(httpCode);
		log_w("HTTP response: %d, %s", httpCode, errStr.c_str());
		return std::make_shared<cal::Error>(cal::Error::Type::HTTP,
		                                    "HTTP: " + String(httpCode) + ", " + errStr);
	}

	DeserializationError err = deserializeJson(doc, responseBody);
	if (err) {
		String errStr = err.f_str();
		log_w("deserializeJson() failed with code %s", errStr.c_str());
		return std::make_shared<cal::Error>(cal::Error::Type::PARSE,
		                                    "deserializeJson() failed with code " + errStr);
	}

	return nullptr;
}
}  // namespace cal