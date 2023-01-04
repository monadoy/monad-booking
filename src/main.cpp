#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>
#include <Preferences.h>
#include <esp_crt_bundle.h>
#include <esp_tls.h>
#include <esp_wifi.h>

#include <memory>

#include "calendar/apiTask.h"
#include "calendar/googleApi.h"
#include "calendar/microsoftApi.h"
#include "calendar/model.h"
#include "globals.h"
#include "gui/guiTask.h"
#include "localization.h"
#include "myUpdate.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "timeUtils.h"
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define USE_EXTERNAL_SERIAL true

#define BOOT_WIFI_CONNECT_MAX_RETRIES 10

#define NTP_TIMEOUT_MS 20 * 1000

#define SETUP_HOLD_BUTTON_MS 10000

std::unique_ptr<config::ConfigStore> configStore = nullptr;
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
		guiTask->showShutdownScreen(message + "\nRetrying automatically in "
		                                + String(ERROR_REBOOT_DELAY_S / 60) + " min...",
		                            true);
	}
	sleepManager.requestErrorReboot();
};

// Firmware update calls this before writing to flash.
// Animation uses flash to load images, so stop it
// to avoid writing and reading at the same time.
void onBeforeFilesystemWrite() {
	guiTask->stopLoading();
	delay(500);  // Delay to make sure that the last frame has loaded.
}

void autoUpdateFirmware() {
	if (latestVersionResult.isErr() || *latestVersionResult.ok() == CURRENT_VERSION)
		return;

	if (utils::getBatteryLevel() < 0.4) {
		log_i("Battery level too low to update firmware");
		return;
	}

	guiTask->setLoadingScreenText("Updating to firmware: v" + *latestVersionResult.ok() + " ("
	                              + UPDATE_CHANNEL + ")\nThis takes a while...");
	auto err = updateFirmware(*latestVersionResult.ok(), UPDATE_CHANNEL, onBeforeFilesystemWrite);
	if (err) {
		// Errors don't really matter here as they aren't fatal
		// TODO: somehow show the error to user
	}
	// Resume normal operation after failure (updateFirmware reboots on success)
	guiTask->startLoading();
}

std::unique_ptr<cal::APITask> createApiTask(JsonObjectConst config) {
	cal::API* api = nullptr;

	const String provider = config["calendar_provider"] | "google";
	const String key = provider == "google" ? "gcalsettings" : "mscalsettings";

	auto tokenRes = cal::jsonToToken(config[key]["token"]);
	if (tokenRes.isErr()) {
		handleBootError(tokenRes.err()->message);
		return nullptr;
	}
	if (provider == "google") {
		api = new cal::GoogleAPI{*tokenRes.ok(), config[key]["calendarid"]};
	} else if (provider == "microsoft") {
		api = new cal::MicrosoftAPI{*tokenRes.ok(), config[key]["room_email"]};
	} else {
		handleBootError("Unknown calendar provider: " + String(provider));
		return nullptr;
	}

	api->registerSaveTokenFunc([key](const cal::Token& token) {
		auto newConfig = DynamicJsonDocument(4096);
		JsonObject newKey = newConfig.createNestedObject(key);
		JsonObject newToken = newKey.createNestedObject("token");
		cal::tokenToJson(newToken, token);

		// log_i("Saving token");
		// token.print();
		serializeJsonPretty(newConfig, Serial);
		configStore->mergeConfig(newConfig);
		log_i("Updated token saved");
	});

	return utils::make_unique<cal::APITask>(std::unique_ptr<cal::API>(api));
}

void normalBoot(JsonObjectConst config) {
	guiTask = utils::make_unique<gui::GUITask>();
	guiTask->startLoading();
	guiTask->setLoadingScreenText("Booting...");

	auto error = l10n.setLanguage(config["language"]);
	if (error) {
		handleBootError(error->message);
		return;
	}

	sleepManager.setOnTimes(config["awake"]);

	if (!wifiManager.openStation(config["wifi"]["ssid"], config["wifi"]["password"],
	                             BOOT_WIFI_CONNECT_MAX_RETRIES)) {
		// Try de-authenticating to fix https://github.com/monadoy/monad-booking/issues/1
		// (not sure if it works)
		if (wifiManager.getDisconnectReason() == WIFI_REASON_AUTH_EXPIRE) {
			esp_wifi_deauth_sta(0);
		}
		handleBootError("WIFI Error: " + wifiManager.getDisconnectReasonString() + ".");
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

	UPDATE_CHANNEL = config["update_channel"] | String("stable");
	latestVersionResult = getLatestFirmwareVersion(UPDATE_CHANNEL);
	if (preferences.getBool(LAST_BOOT_SUCCESS_KEY)
	    && (config["autoupdate"] | false || preferences.getBool(MANUAL_UPDATE_KEY))) {
		preferences.putBool(MANUAL_UPDATE_KEY, false);
		autoUpdateFirmware();
	}

	apiTask = createApiTask(config);
	if (!apiTask) {
		return;  // Error already handled
	}

	calendarModel = utils::make_unique<cal::Model>(*apiTask);
	calendarModel->registerGUITask(guiTask.get());
	guiTask->initMain(calendarModel.get());
	calendarModel->updateStatus();

	// utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339) + "] normal boot");

	preferences.putBool(CURR_BOOT_SUCCESS_KEY, true);
}

bool detectButtonHold() {
	log_i("Detecting button hold");
	// Hold button for 5s to force setup
	M5.update();
	if (M5.BtnP.isPressed()) {
		M5EPD_Canvas canvas(&M5.EPD);
		canvas.createCanvas(960, 200);
		canvas.fillCanvas(0);
		canvas.setTextColor(15);
		canvas.setTextSize(4);
		canvas.setTextDatum(CC_DATUM);
		canvas.drawString("Keep holding the button ", 960 / 2, 80);
		canvas.drawString("to enter setup", 960 / 2, 120);
		canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
	}

	time_t holdStart = millis();

	while (M5.BtnP.isPressed()) {
		if (millis() - holdStart > SETUP_HOLD_BUTTON_MS) {
			return true;
		}
		delay(100);
		M5.update();
	};

	log_i("Not pressed");
	return false;
}

void setupBoot() {
	guiTask = utils::make_unique<gui::GUITask>();
	sleepManager.incrementTaskCounter();
	// Try to sync from rtc in case there is some kind of time
	syncEzTimeFromRTC();

	Serial.println("Starting in setup mode.");
	guiTask = utils::make_unique<gui::GUITask>();
	delay(100);

	guiTask->startSetup(true);

	// utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339)
	//                        + "] setup boot (timestamp unreliable)");
}

void setup() {
	vTaskPrioritySet(NULL, 5);

	esp_tls_cfg_t cfg = {
	    .crt_bundle_attach = esp_crt_bundle_attach,
	};

#ifdef USE_EXTERNAL_SERIAL
	Serial.begin(115200, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_19);
	delay(100);
#endif

	M5.begin(true, false, !USE_EXTERNAL_SERIAL, true, true);
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);
	M5.RTC.begin();

	Serial.println("========== Monad Booking v" + CURRENT_VERSION + " ==========");
	Serial.println("Booting up...");

	if (!preferences.begin("main")) {
		log_e("Preferences begin failed");
	}
	// Cycle boot success booleans
	preferences.putBool(LAST_BOOT_SUCCESS_KEY, preferences.getBool(CURR_BOOT_SUCCESS_KEY));
	preferences.putBool(CURR_BOOT_SUCCESS_KEY, false);
	log_i("Last boot success: %d", preferences.getBool(LAST_BOOT_SUCCESS_KEY));

	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
		log_e("LittleFS Mount Failed");
		return;
	}

	// Default png buffer uses internal ram, this way we can use our psram
	uint8_t* imageBuffer = new uint8_t[PNG_BUFFER_SIZE];
	png.setBuffer(imageBuffer);

	configStore = utils::make_unique<config::ConfigStore>(LittleFS);
	JsonObjectConst config = configStore->getConfigJson();

	bool forceSetup = detectButtonHold();
	if (!forceSetup && config.begin() != config.end()) {
		normalBoot(config);
	} else {
		setupBoot();
	}

	// Serial.println("Boot log: ");
	// auto entries = utils::getBootLog();
	// for (int i = entries.size() - 1; i >= 0; --i) {
	// 	Serial.println(entries[i]);
	// }
}

void loop() { vTaskDelete(NULL); }
