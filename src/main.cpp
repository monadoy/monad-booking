
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <ezTime.h>

#include <memory>

#include "calendar/apiTask.h"
#include "calendar/googleApi.h"
#include "configServer.h"
#include "gui/gui.h"
#include "safeTimezone.h"
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define USE_EXTERNAL_SERIAL true

// Provide official timezone names
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
const char* IANA_TZ = "Europe/Helsinki";

// TODO: Force everyone to use thread safe version
Timezone myTZ;
SafeTimezone* safeMyTZ;
SafeTimezone* safeUTC;

M5EPD_Canvas canvas(&M5.EPD);

#define MICROS_PER_MILLI 1000
#define MILLIS_PER_SEC 1000
#define MICROS_PER_SEC 1000000
const uint64_t UPDATE_STATUS_INTERVAL_MS = 120 * MILLIS_PER_SEC;
const uint64_t WAKE_TIME_MS = 400;

std::array<uint8_t, 2> offDays{SATURDAY, SUNDAY};

std::array<uint8_t, 2> onHours{7, 19};

std::shared_ptr<Config::ConfigStore> configStore = nullptr;

std::shared_ptr<cal::APITask> apiTask = nullptr;

bool restoreWifiConfig();

void setupTime(const String& IANATimeZone) {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);
	ezt::waitForSync();

	if (!myTZ.setCache(String("timezones"), IANATimeZone))
		myTZ.setLocation(IANATimeZone);
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

	safeMyTZ = new SafeTimezone(myTZ);
	safeUTC = new SafeTimezone(UTC);

	configStore = std::make_shared<Config::ConfigStore>(LittleFS);

	Serial.println("Setting up E-ink display...");
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);

	Serial.println("Setting up RTC...");
	M5.RTC.begin();

	JsonObjectConst config = configStore->getConfigJson();

	auto tokenRes = cal::GoogleAPI::parseToken(config["gcalsettings"]["token"]);

	if (tokenRes.isErr()) {
		Serial.println(tokenRes.err()->message);
		return;
	}

	cal::API* api = new cal::GoogleAPI{*tokenRes.ok(), *safeMyTZ, *safeUTC,
	                                   config["gcalsettings"]["calendarid"]};

	apiTask = std::make_shared<cal::APITask>(std::unique_ptr<cal::API>(api));

	esp_wifi_start();
	utils::connectWiFi(config["wifi"]["ssid"], config["wifi"]["password"]);
	setupTime(config["timezone"]);

	utils::addBootLogEntry("[" + myTZ.dateTime(RFC3339) + "] normal boot");

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
	time_t t = safeMyTZ->now();
	tmElements_t tm;
	ezt::breakTime(t, tm);

	return std::any_of(offDays.begin(), offDays.end(), [&](uint8_t d) { return d == tm.Wday; })
	       || tm.Hour < onHours[0] || tm.Hour >= onHours[1];
}

time_t calculateTurnOnTimeUTC() {
	time_t now = safeMyTZ->now();

	tmElements_t tm;
	ezt::breakTime(now, tm);
	if (tm.Hour > onHours[0]) {
		tm.Day += 1;
	}
	tm.Hour = onHours[0];
	tm.Minute = 5;
	tm.Second = 0;

	return safeMyTZ->tzTime(ezt::makeTime(tm));
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

	String turnOnTimeStr = safeMyTZ->dateTime(turnOnTimeUTC, UTC_TIME, RFC3339);

	const String log = "[" + safeMyTZ->dateTime(RFC3339) + "] Shut, try wake at " + turnOnTimeStr;
	Serial.println(log);
	utils::addBootLogEntry(log);

	gui::showBootLog();

	tmElements_t tmOn;
	ezt::breakTime(turnOnTimeUTC, tmOn);

	M5.shutdown(RTC_Date(tmOn.Wday, tmOn.Month, tmOn.Day, tmOn.Year - 1900),
	            RTC_Time(tmOn.Hour, tmOn.Minute, tmOn.Second));
}

// When to stop looping and go back to sleep
unsigned long wakeUntilMillis = 0;

// When to wake up in order to update status
unsigned long nextStatusUpdateMillis = 0;

void sleep() {
	Serial.println("Going to sleep...");

	// Calculate how long we need to sleep in order to achieve the desired update status interval.
	unsigned long curMillis = millis();
	uint64_t sleepTime
	    = uint64_t(max(nextStatusUpdateMillis, curMillis + 1) - curMillis) * MICROS_PER_MILLI;

	Serial.print("Sleeping for ");
	Serial.print(sleepTime / MICROS_PER_SEC);
	Serial.println(" s or until touch.");

	Serial.flush();

	utils::sleepWiFi();

	// Light sleep and wait for timer or touch interrupt
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);  // TOUCH_INT
	esp_sleep_enable_timer_wakeup(sleepTime);
	esp_light_sleep_start();

	Serial.print("Wakeup cause: ");
	auto cause = esp_sleep_get_wakeup_cause();
	if (cause == ESP_SLEEP_WAKEUP_TIMER) {
		Serial.println("TIMER");
		Serial.println("Updating gui status and going back to sleep");

		// When wakeup is caused by timer, a status update is due.
		gui::updateGui();

		// Calculate the next status update timestamp based on our interval.
		nextStatusUpdateMillis = millis() + UPDATE_STATUS_INTERVAL_MS;

		// Go instantly back to sleep
		wakeUntilMillis = 0;
	} else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
		Serial.println("TOUCH");

		// Stay awake for a moment to process long touches
		wakeUntilMillis = millis() + WAKE_TIME_MS;
	}
}

void loop() {
	// esp_event_post("TEST", 0, nullptr, sizeof(void*), 0);
	// delay(100);
	// if (utils::isSetupMode())
	// 	return;

	// gui::loopGui();

	// delay(100);

	// while (millis() >= wakeUntilMillis) {
	// 	// Don't shut down if battery is charging or completely full
	// 	if (shouldShutDown() && M5.getBatteryVoltage() < 4200) {
	// 		shutDown();
	// 	}

	// 	sleep();
	// }
}
