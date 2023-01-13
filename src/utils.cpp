#include "utils.h"

#include <HTTPClient.h>
#include <LittleFS.h>

#include <deque>

namespace utils {
const uint32_t BAT_LOW = 3300;
const uint32_t BAT_HIGH = 4200;

const size_t MAX_BATTERY_HISTORY = 5;

std::deque<uint32_t> batteryVoltageHistory;
time_t lastBatteryVoltageUpdate = 0;
uint32_t getSmoothBatteryVoltage() {
	if (batteryVoltageHistory.size() >= MAX_BATTERY_HISTORY) {
		batteryVoltageHistory.pop_front();
	}
	uint32_t voltage = M5.getBatteryVoltage();
	batteryVoltageHistory.push_back(M5.getBatteryVoltage());

	uint32_t sum = 0;
	for (uint32_t v : batteryVoltageHistory) {
		sum += v;
	}

	uint32_t avg = sum / batteryVoltageHistory.size();

	// Clear history if voltage is too far off from average (e.g. started or stopped charging)
	if (abs((int32_t)voltage - (int32_t)avg) > 100) {
		batteryVoltageHistory.clear();
		return voltage;
	}

	return avg;
}

/**
 * Returns battery level in 0.0-1.0 range.
 */
float getBatteryLevel() {
	auto voltage = getSmoothBatteryVoltage();
	auto clamped = std::min(std::max(getSmoothBatteryVoltage(), BAT_LOW), BAT_HIGH);
	return (float)(clamped - BAT_LOW) / (float)(BAT_HIGH - BAT_LOW);
}

bool isCharging() { return M5.getBatteryVoltage() > 4275; }

std::vector<String> getBootLog() {
	fs::File readHandle = LittleFS.open("/boot.log", FILE_READ);
	if (!readHandle) {
		return std::vector<String>();
	}

	std::vector<String> entries{};
	String line;
	while (true) {
		line = readHandle.readStringUntil('\n');
		if (line.isEmpty())
			break;
		entries.push_back(std::move(line));
	}
	readHandle.close();

	return entries;
}

bool addBootLogEntry(const String& entry) {
	auto entries = getBootLog();

	if (entries.size() > 11) {
		entries.erase(entries.begin());
	}

	entries.push_back(entry);

	fs::File writeHandle = LittleFS.open("/boot.log", FILE_WRITE, true);
	if (!writeHandle) {
		return false;
	}

	for (const String& entry : entries) {
		writeHandle.println(entry);
	}
	writeHandle.close();
	return true;
}

void forceRestart() {
	Serial.flush();

	M5.shutdown(1);

	// Normally m5paper won't allow restarts while USB is plugged in.
	// We induce a crash to shut down for sure.
	String* shutdown = nullptr;
	shutdown->toUpperCase();
}

std::vector<String> listFiles(const String& directory) {
	std::vector<String> files;

	File dir = LittleFS.open(directory);
	if (!dir) {
		log_e("Not a directory: %s", directory.c_str());
		return files;
	}

	log_d("Files in directory  %s:", directory.c_str());

	while (true) {
		File f = dir.openNextFile();
		if (!f) {
			break;
		}
		if (!f.isDirectory()) {
			log_d("File: %s", f.path());
			files.push_back(f.name());
		} else {
			log_d("Dir : %s", f.path());
		}

		f.close();
	}
	dir.close();

	return files;
}

void merge(JsonVariant dst, JsonVariantConst src) {
	if (src.is<JsonObjectConst>()) {
		for (auto kvp : src.as<JsonObjectConst>()) {
			merge(dst.getOrAddMember(kvp.key()), kvp.value());
		}
	} else if (!src.isNull()) {
		dst.set(src);
	}
}

String httpCodeToString(int code) {
	switch (code) {
		case HTTP_CODE_CONTINUE:
			return "continue";
		case HTTP_CODE_SWITCHING_PROTOCOLS:
			return "switching_protocols";
		case HTTP_CODE_PROCESSING:
			return "processing";
		case HTTP_CODE_OK:
			return "ok";
		case HTTP_CODE_CREATED:
			return "created";
		case HTTP_CODE_ACCEPTED:
			return "accepted";
		case HTTP_CODE_NON_AUTHORITATIVE_INFORMATION:
			return "non_authoritative_information";
		case HTTP_CODE_NO_CONTENT:
			return "no_content";
		case HTTP_CODE_RESET_CONTENT:
			return "reset_content";
		case HTTP_CODE_PARTIAL_CONTENT:
			return "partial_content";
		case HTTP_CODE_MULTI_STATUS:
			return "multi_status";
		case HTTP_CODE_ALREADY_REPORTED:
			return "already_reported";
		case HTTP_CODE_IM_USED:
			return "im_used";
		case HTTP_CODE_MULTIPLE_CHOICES:
			return "multiple_choices";
		case HTTP_CODE_MOVED_PERMANENTLY:
			return "moved_permanently";
		case HTTP_CODE_FOUND:
			return "found";
		case HTTP_CODE_SEE_OTHER:
			return "see_other";
		case HTTP_CODE_NOT_MODIFIED:
			return "not_modified";
		case HTTP_CODE_USE_PROXY:
			return "use_proxy";
		case HTTP_CODE_TEMPORARY_REDIRECT:
			return "temporary_redirect";
		case HTTP_CODE_PERMANENT_REDIRECT:
			return "permanent_redirect";
		case HTTP_CODE_BAD_REQUEST:
			return "bad_request";
		case HTTP_CODE_UNAUTHORIZED:
			return "unauthorized";
		case HTTP_CODE_PAYMENT_REQUIRED:
			return "payment_required";
		case HTTP_CODE_FORBIDDEN:
			return "forbidden";
		case HTTP_CODE_NOT_FOUND:
			return "not_found";
		case HTTP_CODE_METHOD_NOT_ALLOWED:
			return "method_not_allowed";
		case HTTP_CODE_NOT_ACCEPTABLE:
			return "not_acceptable";
		case HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED:
			return "proxy_authentication_required";
		case HTTP_CODE_REQUEST_TIMEOUT:
			return "request_timeout";
		case HTTP_CODE_CONFLICT:
			return "conflict";
		case HTTP_CODE_GONE:
			return "gone";
		case HTTP_CODE_LENGTH_REQUIRED:
			return "length_required";
		case HTTP_CODE_PRECONDITION_FAILED:
			return "precondition_failed";
		case HTTP_CODE_PAYLOAD_TOO_LARGE:
			return "payload_too_large";
		case HTTP_CODE_URI_TOO_LONG:
			return "uri_too_long";
		case HTTP_CODE_UNSUPPORTED_MEDIA_TYPE:
			return "unsupported_media_type";
		case HTTP_CODE_RANGE_NOT_SATISFIABLE:
			return "range_not_satisfiable";
		case HTTP_CODE_EXPECTATION_FAILED:
			return "expectation_failed";
		case HTTP_CODE_MISDIRECTED_REQUEST:
			return "misdirected_request";
		case HTTP_CODE_UNPROCESSABLE_ENTITY:
			return "unprocessable_entity";
		case HTTP_CODE_LOCKED:
			return "locked";
		case HTTP_CODE_FAILED_DEPENDENCY:
			return "failed_dependency";
		case HTTP_CODE_UPGRADE_REQUIRED:
			return "upgrade_required";
		case HTTP_CODE_PRECONDITION_REQUIRED:
			return "precondition_required";
		case HTTP_CODE_TOO_MANY_REQUESTS:
			return "too_many_requests";
		case HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE:
			return "request_header_fields_too_large";
		case HTTP_CODE_INTERNAL_SERVER_ERROR:
			return "internal_server_error";
		case HTTP_CODE_NOT_IMPLEMENTED:
			return "not_implemented";
		case HTTP_CODE_BAD_GATEWAY:
			return "bad_gateway";
		case HTTP_CODE_SERVICE_UNAVAILABLE:
			return "service_unavailable";
		case HTTP_CODE_GATEWAY_TIMEOUT:
			return "gateway_timeout";
		case HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED:
			return "http_version_not_supported";
		case HTTP_CODE_VARIANT_ALSO_NEGOTIATES:
			return "variant_also_negotiates";
		case HTTP_CODE_INSUFFICIENT_STORAGE:
			return "insufficient_storage";
		case HTTP_CODE_LOOP_DETECTED:
			return "loop_detected";
		case HTTP_CODE_NOT_EXTENDED:
			return "not_extended";
		case HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED:
			return "network_authentication_required";
		default:
			return "unknown";
	}
}

}  // namespace utils