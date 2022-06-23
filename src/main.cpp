
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>

#include <memory>

#include "calendar/apiTask.h"
#include "calendar/googleApi.h"
#include "calendar/model.h"
#include "configServer.h"
#include "globals.h"
#include "gui/gui.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "timeUtils.h"
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define USE_EXTERNAL_SERIAL true

// Only used in setup mode
std::unique_ptr<Config::ConfigServer> configServer = nullptr;

std::unique_ptr<Config::ConfigStore> configStore = nullptr;
std::unique_ptr<cal::APITask> apiTask = nullptr;
std::unique_ptr<gui::GUITask> guiTask = nullptr;
std::unique_ptr<cal::Model> calendarModel = nullptr;

void setupTime(const String& IANATimeZone) {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);
	ezt::waitForSync();

	if (!safeMyTZ.setCache(String("timezones"), IANATimeZone))
		safeMyTZ.setLocation(IANATimeZone);
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

void setup() {
#ifdef USE_EXTERNAL_SERIAL
	Serial.begin(115200, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_19);
	delay(100);
#endif

	M5.begin(true, false, !USE_EXTERNAL_SERIAL, true, true);

	Serial.println("========== Monad Booking ==========");
	Serial.println("Booting up...");

	Serial.println("Setting up LittleFS...");
	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
		Serial.println("LittleFS Mount Failed");
		Serial.println("Please ensure your partition layout has spiffs partition defined");
		return;
	}

	Serial.println("Setting up E-ink display...");
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);

	Serial.println("Setting up RTC...");
	M5.RTC.begin();

	configStore = utils::make_unique<Config::ConfigStore>(LittleFS);
	JsonObjectConst config = configStore->getConfigJson();

	if (config.begin() != config.end()) {
		Serial.println("Starting in normal mode.");

		if (!wifiManager.openStation(config["wifi"]["ssid"], config["wifi"]["password"])) {
			// TODO: show error on screen
			return;
		}
		setupTime(config["timezone"]);
		syncRTCFromEzTime();
		// ezTime uses millis() and drifts over time, sync it from rtc after every wake from sleep
		sleepManager.registerCallback(SleepManager::Callback::AFTER_WAKE,
		                              []() { syncEzTimeFromRTC(); });

		auto tokenRes = cal::GoogleAPI::parseToken(config["gcalsettings"]["token"]);

		if (tokenRes.isErr()) {
			Serial.println(tokenRes.err()->message);
			return;
		}

		cal::API* api = new cal::GoogleAPI{*tokenRes.ok(), config["gcalsettings"]["calendarid"]};
		apiTask = utils::make_unique<cal::APITask>(std::unique_ptr<cal::API>(api));

		calendarModel = utils::make_unique<cal::Model>(*apiTask);

		guiTask = utils::make_unique<gui::GUITask>();
		calendarModel->registerGUITask(guiTask.get());

		calendarModel->updateStatus();

		utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339) + "] normal boot");
	} else {
		Serial.println("Starting in setup mode.");

		// Try to sync from rtc in case there is some kind of time
		syncEzTimeFromRTC();

		Serial.println("No config stored");
		Serial.println("Entering setup-mode...");

		wifiManager.openAccessPoint();

		auto info = wifiManager.getAccessPointInfo();
		Serial.println(info.ip);

		log_i("\nAPInfo:\nssid: %s\npass: %s\nIP: %s", info.ssid.c_str(), info.password.c_str(),
		      info.ip.toString().c_str());

		// TODO: Refactor gui init to work without model or config
		// guiTask = utils::make_unique<gui::GUITask>();
		// guiTask.showSetupScreen();

		configServer = utils::make_unique<Config::ConfigServer>(80, configStore.get());
		configServer->start();

		utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339)
		                       + "] setup boot (timestamp unreliable)");
	}

	Serial.println("Boot log: ");
	auto entries = utils::getBootLog();
	for (int i = entries.size() - 1; i >= 0; --i) {
		Serial.println(entries[i]);
	}
}

void loop() { vTaskDelete(NULL); }
