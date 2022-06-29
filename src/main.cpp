
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>

#include <memory>

#include "calendar/apiTask.h"
#include "calendar/googleApi.h"
#include "calendar/model.h"
#include "configServer.h"
#include "globals.h"
#include "gui/animManager.h"
#include "gui/gui.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "timeUtils.h"
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define USE_EXTERNAL_SERIAL true

#define BOOT_WIFI_CONNECT_MAX_RETRIES 5

// Only used in setup mode
std::unique_ptr<Config::ConfigServer> configServer = nullptr;

std::unique_ptr<Config::ConfigStore> configStore = nullptr;
std::unique_ptr<cal::APITask> apiTask = nullptr;
std::unique_ptr<gui::GUITask> guiTask = nullptr;
std::unique_ptr<anim::Animation> animation = nullptr;
std::unique_ptr<cal::Model> calendarModel = nullptr;

void initAwakeTimes(JsonObjectConst config) {
	const JsonObjectConst onDaysJson = config["awake"]["weekdays"];
	const JsonObjectConst onTimesJson = config["awake"]["time"];
	const String onTimeFrom = onTimesJson["from"];
	const String onTimeTo = onTimesJson["to"];

	const std::array<bool, 7> onDays{onDaysJson["sun"], onDaysJson["mon"], onDaysJson["tue"],
	                                 onDaysJson["wed"], onDaysJson["thu"], onDaysJson["fri"],
	                                 onDaysJson["sat"]};
	const std::array<uint8_t, 2> onHours{(uint8_t)onTimeFrom.substring(0, 2).toInt(),
	                                     (uint8_t)onTimeTo.substring(0, 2).toInt()};
	const std::array<uint8_t, 2> onMinutes{(uint8_t)onTimeFrom.substring(2, 4).toInt(),
	                                       (uint8_t)onTimeTo.substring(2, 4).toInt()};

	sleepManager.setOnTimes(onDays, onHours, onMinutes);
}

bool setupTime(const String& IANATimeZone) {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);
	if (!ezt::waitForSync(10)) {
		return false;
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

void setup() {
#ifdef USE_EXTERNAL_SERIAL
	Serial.begin(115200, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_19);
	delay(100);
#endif

	M5.begin(true, false, !USE_EXTERNAL_SERIAL, true, true);
	vTaskPrioritySet(NULL, 5);

	Serial.println("========== Monad Booking ==========");
	Serial.println("Booting up...");

	Serial.println("Setting up LittleFS...");
	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
		Serial.println("LittleFS Mount Failed");
		Serial.println("Please ensure your partition layout has spiffs partition defined");
		return;
	}
	animation = utils::make_unique<anim::Animation>();
	Serial.println("Setting up E-ink display...");
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);  // TODO: move debug texts to startup text

	Serial.println("Setting up RTC...");
	M5.RTC.begin();
	guiTask = utils::make_unique<gui::GUITask>();
	gui::registerAnimation(animation.get());
	guiTask->startLoading();
	guiTask->showLoadingText("            Käynnistetään...");

	configStore = utils::make_unique<Config::ConfigStore>(LittleFS);
	JsonObjectConst config = configStore->getConfigJson();

	if (config.begin() != config.end()) {
		initAwakeTimes(config);

		if (!wifiManager.openStation(config["wifi"]["ssid"], config["wifi"]["password"],
		                             BOOT_WIFI_CONNECT_MAX_RETRIES)) {
			log_i("Didn't get internet access during boot up sequence, shutting down...");
			guiTask->showLoadingText("Couldn't connect WIFI:\n" + wifiManager.getDisconnectReason()
			                         + ".");
			guiTask->stopLoading();
			return;
		}
		if (!setupTime(config["timezone"])) {
			log_i("Couldn't sync with NTP server, shutting down...");
			guiTask->showLoadingText("Couldn't sync with NTP server.");
			guiTask->stopLoading();
			return;
		}

		syncRTCFromEzTime();
		// ezTime uses millis() and drifts over time, sync it from rtc after every wake from sleep
		sleepManager.registerCallback(SleepManager::Callback::AFTER_WAKE,
		                              []() { syncEzTimeFromRTC(); });

		auto tokenRes = cal::GoogleAPI::parseToken(config["gcalsettings"]["token"]);

		if (tokenRes.isErr()) {
			guiTask->showLoadingText(tokenRes.err()->message);
			guiTask->stopLoading();
			return;
		}

		cal::API* api = new cal::GoogleAPI{*tokenRes.ok(), config["gcalsettings"]["calendarid"]};
		apiTask = utils::make_unique<cal::APITask>(std::unique_ptr<cal::API>(api));

		calendarModel = utils::make_unique<cal::Model>(*apiTask);

		guiTask->initMain(calendarModel.get());

		calendarModel->registerGUITask(guiTask.get());
		calendarModel->updateStatus();

		utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339) + "] normal boot");
	} else {
		Serial.println("Starting in setup mode.");

		// Try to sync from rtc in case there is some kind of time
		syncEzTimeFromRTC();

		wifiManager.openAccessPoint();

		auto info = wifiManager.getAccessPointInfo();
		Serial.println(info.ip);

		log_i("\nAPInfo:\nssid: %s\npass: %s\nIP: %s", info.ssid.c_str(), info.password.c_str(),
		      info.ip.toString().c_str());

		guiTask->stopLoading();

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
