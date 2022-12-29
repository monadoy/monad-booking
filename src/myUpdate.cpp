#include "myUpdate.h"

#include <Arduino.h>
#define DEST_FS_USES_LITTLEFS
#include <ESP32-targz.h>
#include <HTTPClient.h>
#include <HttpsOTAUpdate.h>
#include <M5EPD.h>
#include <cert.h>

#include "esp_ota_ops.h"
#include "globals.h"
#include "utils.h"

#define UPDATE_SERVER_CERT GOOGLE_API_FULL_CHAIN_CERT
#define UPDATE_STORAGE_URL "https://storage.googleapis.com/no-booking-binaries"

String getUrlBase(const String& channel) {
	return String(UPDATE_STORAGE_URL) + (channel == "beta" ? "/beta" : "");
}

utils::Result<String> getLatestFirmwareVersion(const String& channel) {
	HTTPClient http;
	http.setReuse(false);

	http.begin(getUrlBase(channel) + "/current-version");

	const int httpCode = http.GET();

	if (httpCode != 200) {
		log_e("http returned code %d", httpCode);
		http.end();
		return utils::Result<String>::makeErr(
		    new utils::Error("HTTP returned: " + String(httpCode)));
	}

	const String version = http.getString();
	http.end();

	if (version.length() < 1 || version[0] < '0' || version[0] > '9') {
		return utils::Result<String>::makeErr(
		    new utils::Error("Invalid version string: " + version));
	}

	log_i("Detected latest version: %s", version.c_str());

	return utils::Result<String>::makeOk(new String(version));
}

std::unique_ptr<utils::Error> downloadUpdateFile(const String& url, const String& filename) {
	auto startTime = millis();

	HTTPClient http;
	http.setReuse(false);

	http.begin(url);

	const int httpCode = http.GET();

	if (httpCode != 200) {
		log_e("http returned code %d", httpCode);
		http.end();
		return utils::make_unique<utils::Error>("Firmware updater HTTP returned "
		                                        + String(httpCode));
	}

	File file = LittleFS.open(filename, "w", true);

	if (!file) {
		http.end();
		return utils::make_unique<utils::Error>("Firmware updater couldn't open file " + filename);
	}

	int bytes = http.writeToStream(&file);
	http.end();
	if (bytes < 0) {
		file.close();
		return utils::make_unique<utils::Error>("Firmware updater file write failed, code: "
		                                        + String(bytes));
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

std::unique_ptr<utils::Error> updateFirmware(const String& newVersion, const String& channel,
                                             const std::function<void()> onBeforeFormat) {
	auto count = sleepManager.scopedTaskCount();

	LittleFS.rmdir("/tmp");

	log_i("Downloading new filesystem...");
	auto err = downloadUpdateFile(getUrlBase(channel) + "/v" + newVersion + "/file-system.tar.gz",
	                              "/tmp/file-system.tar.gz");
	if (err)
		return err;

	const String firmwareUpdateUrl = getUrlBase(channel) + "/v" + newVersion + "/firmware.bin";

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

	// Move old assets to tmp.
	// This is needed because assets are fingerprinted, so they are not overwritten when expanding
	// filesystem and would take a lot of space after multiple updates.
	auto assets = utils::listFiles("/webroot/assets");
	for (const auto& file : assets) {
		LittleFS.rename("/webroot/assets/" + file, "/tmp/webroot/assets/" + file);
	}

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

		// Revert asset move
		for (const auto& file : assets) {
			LittleFS.rename("/tmp/webroot/assets/" + file, "/webroot/assets/" + file);
		}

		return utils::make_unique<utils::Error>("Firmware updater failed.");
	}
	TARGZUnpacker.reset();

	log_i("Firmware and filesystem update success. Restarting...");

	utils::forceRestart();

	// This is just to silence compiler warnings.
	return nullptr;
}
