
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
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define USE_EXTERNAL_SERIAL true

std::array<uint8_t, 2> offDays{SATURDAY, SUNDAY};

std::array<uint8_t, 2> onHours{7, 19};

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

	wifiManager.openStation(config["wifi"]["ssid"], config["wifi"]["password"]);
	setupTime(config["timezone"]);

	auto tokenRes = cal::GoogleAPI::parseToken(config["gcalsettings"]["token"]);

	if (tokenRes.isErr()) {
		Serial.println(tokenRes.err()->message);
		return;
	}

	cal::API* api = new cal::GoogleAPI{*tokenRes.ok(), config["gcalsettings"]["calendarid"]};

	apiTask = utils::make_unique<cal::APITask>(std::unique_ptr<cal::API>(api));

	calendarModel = utils::make_unique<cal::Model>(*apiTask);
	guiTask = utils::make_unique<gui::GUITask>(configStore.get(), calendarModel.get());
	calendarModel->registerGUITask(guiTask.get());

	calendarModel->updateStatus();

	utils::addBootLogEntry("[" + safeMyTZ.dateTime(RFC3339) + "] normal boot");

	// 	gui::initGui(&myTZ, configStore.get());

	// SETUPMODE CODE
	// utils::addBootLogEntry("[UNKNOWN] setup boot");
	// Serial.println("No config stored");
	// Serial.println("Entering setup-mode...");
	// utils::setupMode();
	// gui::toSetupScreen();
	// Initialize Configserver
	// Config::ConfigServer* configServer = new Config::ConfigServer(80, configStore);
	// configServer->start();

	Serial.println("Boot log: ");
	auto entries = utils::getBootLog();
	for (int i = entries.size() - 1; i >= 0; --i) {
		Serial.println(entries[i]);
	}
}

bool shouldShutDown() {
	time_t t = safeMyTZ.now();
	tmElements_t tm;
	ezt::breakTime(t, tm);

	return std::any_of(offDays.begin(), offDays.end(), [&](uint8_t d) { return d == tm.Wday; })
	       || tm.Hour < onHours[0] || tm.Hour >= onHours[1];
}

time_t calculateTurnOnTimeUTC() {
	time_t now = safeMyTZ.now();

	tmElements_t tm;
	ezt::breakTime(now, tm);
	if (tm.Hour > onHours[0]) {
		tm.Day += 1;
	}
	tm.Hour = onHours[0];
	tm.Minute = 5;
	tm.Second = 0;

	return safeMyTZ.tzTime(ezt::makeTime(tm));
}

void shutDown() {
	time_t nowUTC = UTC.now();

	tmElements_t tm;
	ezt::breakTime(nowUTC, tm);

	RTC_Date date(tm.Wday, tm.Month, tm.Day, tm.Year - 1900);
	RTC_Time time(tm.Hour, tm.Minute, tm.Second);
	M5.RTC.setDate(&date);
	M5.RTC.setTime(&time);

	time_t turnOnTimeUTC = calculateTurnOnTimeUTC();

	String turnOnTimeStr = safeMyTZ.dateTime(turnOnTimeUTC, UTC_TIME, RFC3339);

	const String log = "[" + safeMyTZ.dateTime(RFC3339) + "] Shut, try wake at " + turnOnTimeStr;
	Serial.println(log);
	utils::addBootLogEntry(log);

	gui::showBootLog();

	tmElements_t tmOn;
	ezt::breakTime(turnOnTimeUTC, tmOn);

	M5.shutdown(RTC_Date(tmOn.Wday, tmOn.Month, tmOn.Day, tmOn.Year - 1900),
	            RTC_Time(tmOn.Hour, tmOn.Minute, tmOn.Second));
}

void loop() { vTaskDelete(NULL); }
