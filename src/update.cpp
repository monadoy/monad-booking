#include "update.h"

#include <Arduino.h>
#define DEST_FS_USES_LITTLEFS
#include <ESP32-targz.h>
#include <HTTPClient.h>
#include <HttpsOTAUpdate.h>
#include <M5EPD.h>

#include "cert.h"
#include "globals.h"
#include "utils.h"

#define UPDATE_SERVER_CERT GOOGLE_API_FULL_CHAIN_CERT
#define UPDATE_STORAGE_URL "https://storage.googleapis.com/no-booking-binaries"

/**
 * Parses at least three numbers from the given string, separated by periods.
 * Even if the string doesn't contain three or more numbers,
 * zeros are added until there are three numbers.
 */
std::vector<int> parseVersionParts(String version) {
	version.trim();

	std::vector<int> versionParts;
	String current;
	for (const char c : version) {
		if (c == '.') {
			versionParts.push_back(current.toInt());
			current = "";
		} else if (isdigit(c)) {
			current += c;
		}
	}
	versionParts.push_back(current.toInt());

	// If no version numbers were found, just default to zeros
	while (versionParts.size() < 3) {
		versionParts.push_back(0);
	}

	return versionParts;
}

String versionToString(std::array<int, 3> version) {
	return String(version[0]) + "." + String(version[1]) + "." + String(version[2]);
}

std::array<int, 3> getAvailableFirmwareVersion() {
	HTTPClient http;
	http.setReuse(false);
	http.begin(String(UPDATE_STORAGE_URL) + "/LATEST_VERSION", UPDATE_SERVER_CERT);

	const int httpCode = http.GET();

	// TODO: better error handling
	if (httpCode != 200) {
		log_e("http returned code %d", httpCode);
		http.end();
		return {0, 0, 0};
	}

	const String responseBody = http.getString();
	http.end();

	log_i("Response body: %s", responseBody.c_str());

	const std::vector<int> versionParts = parseVersionParts(responseBody);

	log_i("Detected latest version: %d.%d.%d", versionParts[0], versionParts[1], versionParts[2]);

	return {versionParts[0], versionParts[1], versionParts[2]};
}

bool isNewer(std::array<int, 3> newVersion) {
	return VERSION_MAJOR < newVersion[0]
	       || (VERSION_MAJOR == newVersion[0] && VERSION_MINOR < newVersion[1])
	       || (VERSION_MAJOR == newVersion[0] && VERSION_MINOR == newVersion[1]
	           && VERSION_PATCH < newVersion[2]);
}

std::unique_ptr<utils::Error> downloadUpdateFile(const String& url, const String& filename) {
	auto startTime = millis();

	HTTPClient http;
	http.setReuse(false);

	http.begin(url, UPDATE_SERVER_CERT);

	const int httpCode = http.GET();

	// TODO: better error handling
	if (httpCode != 200) {
		log_e("http returned code %d", httpCode);
		http.end();
		return utils::make_unique<utils::Error>("Firmware updater HTTP returned "
		                                        + String(httpCode));
	}

	File file = LittleFS.open(filename, "w", true);

	if (!file) {
		log_e("Could not open %s", filename.c_str());
		return utils::make_unique<utils::Error>("Firmware updater couldn't open file " + filename);
	}

	http.writeToStream(&file);

	log_i("Download done in %u ms", millis() - startTime);

	file.close();

	return nullptr;
}

std::unique_ptr<TarGzUnpacker> makeTarGzUnpacker() {
	auto unpacker = utils::make_unique<TarGzUnpacker>();

	// stop on fail (manual restart/reset required)
	unpacker->haltOnError(true);
	// true = enables health checks but slows down the overall process
	unpacker->setTarVerify(true);
	// prevent the partition from exploding, recommended
	unpacker->setupFSCallbacks(targzTotalBytesFn, targzFreeBytesFn);
	// targzNullProgressCallback or defaultProgressCallback
	unpacker->setGzProgressCallback(BaseUnpacker::defaultProgressCallback);
	// gz log verbosity
	unpacker->setLoggerCallback(BaseUnpacker::targzPrintLoggerCallback);
	// prints the untarring progress for each individual file
	unpacker->setTarProgressCallback(BaseUnpacker::defaultProgressCallback);
	// print the filenames as they're expanded
	unpacker->setTarStatusProgressCallback(BaseUnpacker::defaultTarStatusProgressCallback);
	// tar log verbosity
	unpacker->setTarMessageCallback(BaseUnpacker::targzPrintLoggerCallback);

	return unpacker;
}

void HttpEvent(HttpEvent_t* event) {
	switch (event->event_id) {
		case HTTP_EVENT_ERROR:
			Serial.println("Http Event Error");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			Serial.println("Http Event On Connected");
			break;
		case HTTP_EVENT_HEADER_SENT:
			Serial.println("Http Event Header Sent");
			break;
		case HTTP_EVENT_ON_HEADER:
			Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key,
			              event->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			Serial.print(".");
			break;
		case HTTP_EVENT_ON_FINISH:
			Serial.println("Http Event On Finish");
			break;
		case HTTP_EVENT_DISCONNECTED:
			Serial.println("Http Event Disconnected");
			break;
	}
}

std::unique_ptr<utils::Error> updateFirmware(std::array<int, 3> newVersion,
                                             std::function<void()> onBeforeFormat) {
	auto count = sleepManager.scopedTaskCount();

	LittleFS.rmdir("/tmp");

	log_i("Downloading new filesystem...");
	const String fileSystemUpdateUrl
	    = String(UPDATE_STORAGE_URL) + "/v" + versionToString(newVersion) + "/file-system.tar.gz";
	auto err = downloadUpdateFile(fileSystemUpdateUrl, "/tmp/file-system.tar.gz");
	if (err)
		return err;

	const String firmwareUpdateUrl
	    = String(UPDATE_STORAGE_URL) + "/v" + versionToString(newVersion) + "/firmware.bin";

	log_i("Downloading and updating firmware...");
	HttpsOTA.onHttpEvent(HttpEvent);
	HttpsOTA.begin(firmwareUpdateUrl.c_str(), UPDATE_SERVER_CERT);

	while (true) {
		auto status = HttpsOTA.status();
		if (status == HTTPS_OTA_SUCCESS) {
			log_i("Firmware updated successfully.");
			break;
		} else if (status == HTTPS_OTA_FAIL) {
			log_e("Firmware update failed.");
			return utils::make_unique<utils::Error>(
			    "Firmware updater failed to update new firmware");
		}
		delay(100);
	}

	log_i("Updating filesystem...");

	std::unique_ptr<TarGzUnpacker> TARGZUnpacker = makeTarGzUnpacker();

	// Expand new file system
	if (!TARGZUnpacker->tarGzExpander(tarGzFS, "/tmp/file-system.tar.gz", tarGzFS, "/", nullptr)) {
		log_e("tarGzExpander+intermediate file failed with return code #%d\n",
		      TARGZUnpacker->tarGzGetError());
		return utils::make_unique<utils::Error>(
		    "Failed to expand new filesystem. Device won't work properly now.");
	}
	TARGZUnpacker.reset();

	log_i("Firmware and filesystem update success. Restarting...");

	Serial.flush();

	M5.shutdown(1);

	// Normally m5paper won't allow restarts while USB is plugged in.
	// We induce a crash to shut down for sure.
	String* shutdown = nullptr;
	shutdown->toUpperCase();

	// This is just to silence compiler warnings.
	return nullptr;
}
