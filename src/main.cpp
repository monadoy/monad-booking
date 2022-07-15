
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>
#include <Preferences.h>

#include <memory>

#include "calendar/apiTask.h"
#include "calendar/googleApi.h"
#include "calendar/model.h"
#include "configServer.h"
#include "globals.h"
#include "gui/animManager.h"
#include "gui/gui.h"
#include "localization.h"
#include "myUpdate.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "timeUtils.h"
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define USE_EXTERNAL_SERIAL true

#define BOOT_WIFI_CONNECT_MAX_RETRIES 5

#define NTP_TIMEOUT_MS 20 * 1000

// Only used in setup mode
std::unique_ptr<Config::ConfigServer> configServer = nullptr;

std::unique_ptr<Config::ConfigStore> configStore = nullptr;
std::unique_ptr<cal::APITask> apiTask = nullptr;
std::unique_ptr<gui::GUITask> guiTask = nullptr;
std::unique_ptr<cal::Model> calendarModel = nullptr;

bool setupTime(const String& IANATimeZone) {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);

	auto start = millis();
	while (true) {
		ezt::updateNTP();
		if (millis() - start > NTP_TIMEOUT_MS) {
			return false;  // failure
		}
		if (UTC.year() > 2000) {
			break;  // success
		}
		delay(2000);  // retry
	}

	if (!safeMyTZ.setCache(String("timezones"), IANATimeZone))
		safeMyTZ.setLocation(IANATimeZone);

	return true;
}

void syncEzTimeFromRTC() {
	timeutils::RTCDateTime rtcTime;
	M5.RTC.getDate(&rtcTime.date);
	M5.RTC.getTime(&rtcTime.time);
	safeUTC.setTime(timeutils::toUnixTime(rtcTime));
	log_i("ezTime synced from rtc: %s", safeMyTZ.dateTime(RFC3339).c_str());
}

void syncRTCFromEzTime() {
	timeutils::RTCDateTime dateTime = timeutils::toRTCTime(safeUTC.now());
	M5.RTC.setDate(&dateTime.date);
	M5.RTC.setTime(&dateTime.time);
}

void handleBootError(const String& message) {
	// Try to sync from rtc in case there is some kind of time
	syncEzTimeFromRTC();
	log_e("%s", message.c_str());
	if (guiTask) {
		guiTask->showLoadingText(message + " Reboot to retry.");
		guiTask->stopLoading();
	}
	sleepManager.requestShutdown();
};

// Firmware update calls this before writing to flash.
// Animation uses flash to load images, so stop it
// to avoid writing and reading at the same time.
void onBeforeFilesystemWrite() {
	guiTask->stopLoading();
	delay(500);  // Delay to make sure that the last frame has loaded.
}

void autoUpdateFirmware() {
	if (latestVersionResult.isOk() && *latestVersionResult.ok() != CURRENT_VERSION) {
		guiTask->showLoadingText("Updating to firmware: v" + latestVersionResult.ok()->toString()
		                         + ". This takes a while...");
		auto err = updateFirmware(*latestVersionResult.ok(), onBeforeFilesystemWrite);
		if (err) {
			// Errors don't really matter here as they aren't fatal
			// TODO: somehow show the error to user
		}
		// Resume normal operation after failure (updateFirmware reboots on success)
		guiTask->startLoading();
	}
}

void normalBoot(JsonObjectConst config) {
	guiTask = utils::make_unique<gui::GUITask>();
	guiTask->startLoading();
	guiTask->showLoadingText("Booting...");

	auto error = l10n.setLanguage(config["language"]);
	if (error) {
		handleBootError(error->message);
		return;
	}

	sleepManager.setOnTimes(config["awake"]);

	if (!wifiManager.openStation(config["wifi"]["ssid"], config["wifi"]["password"],
	                             BOOT_WIFI_CONNECT_MAX_RETRIES)) {
		handleBootError("WIFI Error: " + wifiManager.getDisconnectReason() + ".");
		return;
	}

	if (!setupTime(config["timezone"])) {
		handleBootError("Couldn't sync with NTP server.");
		return;
	}

	syncRTCFromEzTime();
	// ezTime uses millis() and drifts over time, sync it from rtc after every wake from sleep
	sleepManager.registerCallback(SleepManager::Callback::AFTER_WAKE,
	                              []() { syncEzTimeFromRTC(); });

	latestVersionResult = getLatestFirmwareVersion();
	if (preferences.getBool(LAST_BOOT_SUCCESS_KEY, false)
	    && (config["autoupdate"] | false || preferences.getBool(FORCE_UPDATE_KEY, false))) {
		preferences.putBool(FORCE_UPDATE_KEY, false);
		autoUpdateFirmware();
	}

	auto tokenRes
	    = cal::GoogleAPI::parseToken(config["gcalsettings"]["token"].as<JsonObjectConst>());
	if (tokenRes.isErr()) {
		handleBootError(tokenRes.err()->message);
		return;
	}
	cal::API* api = new cal::GoogleAPI{*tokenRes.ok(), config["gcalsettings"]["calendarid"]};
	apiTask = utils::make_unique<cal::APITask>(std::unique_ptr<cal::API>(api));

	calendarModel = utils::make_unique<cal::Model>(*apiTask);
	calendarModel->registerGUITask(guiTask.get());
	guiTask->initMain(calendarModel.get());
	calendarModel->updateStatus();

	guiTask->showLoadingText("");
	utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339) + "] normal boot");

	preferences.putBool(CURR_BOOT_SUCCESS_KEY, true);
}

void setupBoot() {
	sleepManager.incrementTaskCounter();
	// Try to sync from rtc in case there is some kind of time
	syncEzTimeFromRTC();

	Serial.println("Starting in setup mode.");
	guiTask = utils::make_unique<gui::GUITask>();
	delay(100);

	wifiManager.openAccessPoint();

	configServer = utils::make_unique<Config::ConfigServer>(80, configStore.get());
	configServer->start();
	guiTask->goSetup(true);

	utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339)
	                       + "] setup boot (timestamp unreliable)");
}

void setup() {
	vTaskPrioritySet(NULL, 5);

#ifdef USE_EXTERNAL_SERIAL
	Serial.begin(115200, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_19);
	delay(100);
#endif

	M5.begin(true, false, !USE_EXTERNAL_SERIAL, true, true);
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);
	M5.RTC.begin();

	Serial.println("========== Monad Booking v" + CURRENT_VERSION.toString() + " ==========");
	Serial.println("Booting up...");

	if (!preferences.begin("main")) {
		log_e("Preferences begin failed");
	}
	// Cycle boot success booleans
	preferences.putBool(LAST_BOOT_SUCCESS_KEY, preferences.getBool(CURR_BOOT_SUCCESS_KEY, false));
	preferences.putBool(CURR_BOOT_SUCCESS_KEY, false);
	log_i("Last boot success: %d", preferences.getBool(LAST_BOOT_SUCCESS_KEY));

	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
		log_e("LittleFS Mount Failed");
		return;
	}

	configStore = utils::make_unique<Config::ConfigStore>(LittleFS);
	JsonObjectConst config = configStore->getConfigJson();

	if (config.begin() != config.end()) {
		normalBoot(config);
	} else {
		setupBoot();
	}

	Serial.println("Boot log: ");
	auto entries = utils::getBootLog();
	for (int i = entries.size() - 1; i >= 0; --i) {
		Serial.println(entries[i]);
	}
}

void loop() { vTaskDelete(NULL); }
