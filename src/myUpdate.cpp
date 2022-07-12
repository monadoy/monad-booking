#include "myUpdate.h"

#include <Arduino.h>
#define DEST_FS_USES_LITTLEFS
#include <ESP32-targz.h>
#include <ESPHTTPClient.h>
#include <HttpsOTAUpdate.h>
#include <M5EPD.h>

#include "cert.h"
#include "esp_ota_ops.h"
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
	ESPHTTPClient http;
	String url = String(UPDATE_STORAGE_URL) + "/current-version";
	http.open(HTTP_METHOD_GET, url.c_str(), UPDATE_SERVER_CERT);

	String responseBody;
	utils::Result<int> httpCodeRes = http.run(responseBody);

	if (httpCodeRes.isErr()) {
		log_e("%s", httpCodeRes.err()->message.c_str());
		return {0, 0, 0};
	}
	if (*httpCodeRes.ok() < 200 || *httpCodeRes.ok() >= 300) {
		log_e("HTTP returned %d", *httpCodeRes.ok());
		return {0, 0, 0};
	}

	log_i("Response body: %s", responseBody.c_str());

	const std::vector<int> versionParts = parseVersionParts(responseBody);

	log_i("Detected latest version: %d.%d.%d", versionParts[0], versionParts[1], versionParts[2]);

	return {versionParts[0], versionParts[1], versionParts[2]};
}

bool isVersionDifferent(std::array<int, 3> newVersion) {
	// All zeros means invalid version
	if (newVersion[0] == 0 && newVersion[1] == 0 && newVersion[2] == 0) {
		return false;
	}

	return VERSION_MAJOR != newVersion[0] || VERSION_MINOR != newVersion[1]
	       || VERSION_PATCH != newVersion[2];
}

std::unique_ptr<utils::Error> downloadUpdateFile(const String& url, const String& filename) {
	auto startTime = millis();

	ESPHTTPClient http;

	http.open(HTTP_METHOD_GET, url.c_str(), UPDATE_SERVER_CERT);

	File file = LittleFS.open(filename, "w", true);

	if (!file) {
		return utils::make_unique<utils::Error>("Firmware updater couldn't open file " + filename);
	}

	utils::Result<int> httpCodeRes = http.run(file);

	if (httpCodeRes.isErr()) {
		file.close();
		log_e("%s", httpCodeRes.err()->message.c_str());
		return utils::make_unique<utils::Error>("Firmware updater: " + httpCodeRes.err()->message);
	}
	if (*httpCodeRes.ok() < 200 || *httpCodeRes.ok() >= 300) {
		file.close();
		log_e("HTTP returned %d", *httpCodeRes.ok());
		return utils::make_unique<utils::Error>("Firmware updater: HTTP returned "
		                                        + String(*httpCodeRes.ok()));
	}

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

void restart() {
	Serial.flush();

	M5.shutdown(1);

	// Normally m5paper won't allow restarts while USB is plugged in.
	// We induce a crash to shut down for sure.
	String* shutdown = nullptr;
	shutdown->toUpperCase();
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
			return utils::make_unique<utils::Error>("Firmware updater failed.");
		}
		delay(100);
	}

	log_i("Updating filesystem...");

	onBeforeFormat();

	std::unique_ptr<TarGzUnpacker> TARGZUnpacker = makeTarGzUnpacker();

	// Expand new file system
	if (!TARGZUnpacker->tarGzExpander(tarGzFS, "/tmp/file-system.tar.gz", tarGzFS, "/", nullptr)) {
		log_e("tarGzExpander+intermediate file failed with return code #%d\n",
		      TARGZUnpacker->tarGzGetError());
		log_e("Filesystem update failed. Reverting firmware...");
		// Revert update
		const esp_partition_t* running_part = esp_ota_get_running_partition();
		esp_ota_set_boot_partition(running_part);
		TARGZUnpacker.reset();
		return utils::make_unique<utils::Error>("Firmware updater failed.");
	}
	TARGZUnpacker.reset();

	log_i("Firmware and filesystem update success. Restarting...");

	restart();

	// This is just to silence compiler warnings.
	return nullptr;
}
