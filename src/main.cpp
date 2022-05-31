
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <ezTime.h>

#include <memory>

#include "configServer.h"
#include "gui.h"
#include "utils.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define CONFIG_NAME "configuration"

// Uncomment this to load config variables from secrets.h
#define DEVMODE 1

#define USE_EXTERNAL_SERIAL true

// Provide official timezone names
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
const char* IANA_TZ = "Europe/Helsinki";
Timezone myTZ;

M5EPD_Canvas canvas(&M5.EPD);

const IPAddress SETUP_IP_ADDR(192, 168, 69, 1);
const char* SETUP_SSID = "BOOKING_SETUP";
const char* PARAM_MESSAGE = "message";

// Config store
Preferences preferences;

bool isSetupMode = false;

#define MICROS_PER_MILLI 1000
#define MILLIS_PER_SEC 1000
#define MICROS_PER_SEC 1000000
const uint64_t UPDATE_STATUS_INTERVAL_MS = 120 * MILLIS_PER_SEC;
const uint64_t WAKE_TIME_MS = 400;

std::array<uint8_t, 2> offDays{SATURDAY, SUNDAY};

std::array<uint8_t, 2> onHours{7, 19};

std::shared_ptr<Config::ConfigStore> configStore = nullptr;

bool restoreWifiConfig();
void setupMode();

struct tm timeinfo;

void setupTime(const String& IANATimeZone) {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);
	ezt::waitForSync();

	if (!myTZ.setCache(String("timezones"), IANATimeZone))
		myTZ.setLocation(IANATimeZone);
}

void printLocalTime() { Serial.println(myTZ.dateTime(RFC3339)); }

void setup() {
#ifdef USE_EXTERNAL_SERIAL
	Serial.begin(115200, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_19);
	delay(100);
#endif

	M5.begin(true, false, !USE_EXTERNAL_SERIAL, true, true);

	Serial.println("========== Monad Booking ==========");
	Serial.println("Booting up...");
	preferences.begin(CONFIG_NAME);

	Serial.println("Setting up LittleFS...");
	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
		Serial.println("LittleFS Mount Failed");
		Serial.println("Please ensure your partition layout has spiffs partition defined");
		return;
	}

	configStore = std::make_shared<Config::ConfigStore>(LittleFS);

	Serial.println("Setting up E-ink display...");
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);

	Serial.println("Setting up RTC...");
	M5.RTC.begin();

	Serial.println("Loading Configuration...");

	JsonObjectConst config = configStore->getConfigJson();

	if (config.begin() != config.end()) {
		Serial.println("Config loaded!");
		esp_wifi_start();
		utils::connectWiFi(config["wifi"]["ssid"], config["wifi"]["password"]);
		setupTime(config["timezone"]);
		gui::initGui(&myTZ, configStore.get());
		delay(3000);
	} else {
		isSetupMode = true;
		Serial.println("No config stored");
		Serial.println("Entering setup-mode...");
		// Setupmode
		setupMode();
		// Initialize Configserver
		// TODO: this is currently thrown away after setup() ends
		Config::ConfigServer* configServer = new Config::ConfigServer(80, configStore);

		configServer->start();
	}
}

void setupMode() {
	Serial.printf("Starting Access Point at \"%s\"\n", SETUP_SSID);
	WiFi.disconnect();
	delay(100);
	WiFi.softAPConfig(SETUP_IP_ADDR, SETUP_IP_ADDR, IPAddress(255, 255, 255, 0));
	WiFi.softAP(SETUP_SSID);
	WiFi.mode(WIFI_MODE_AP);
}

bool shouldShutDown() {
	time_t t = myTZ.now();
	tmElements_t tm;
	ezt::breakTime(t, tm);

	return std::any_of(offDays.begin(), offDays.end(), [&](uint8_t d) { return d == tm.Day; })
	       || tm.Hour < onHours[0] || tm.Hour >= onHours[1];
}

time_t calculateTurnOnTimeUTC() {
	time_t now = myTZ.now();

	tmElements_t tm;
	ezt::breakTime(now, tm);
	tm.Hour = onHours[0];
	tm.Minute = 5;
	tm.Second = 0;
	tm.Day += 1;

	return myTZ.tzTime(ezt::makeTime(tm));
}

void shutDown() {
	time_t nowUTC = UTC.now();

	tmElements_t tm;
	ezt::breakTime(nowUTC, tm);

	RTC_Date date(tm.Wday, tm.Month, tm.Day, tm.Year);
	RTC_Time time(tm.Hour, tm.Minute, tm.Second);

	M5.RTC.setDate(&date);
	M5.RTC.setTime(&time);

	time_t turnOnTimeUTC = calculateTurnOnTimeUTC();

	String turnOnTimeStr = myTZ.dateTime(turnOnTimeUTC, UTC_TIME, RFC3339);

	Serial.println("Shut down, wakes up at " + turnOnTimeStr);

	M5.EPD.Active();
	M5.EPD.Clear(true);
	canvas.createCanvas(960, 540);
	canvas.setTextSize(24);
	canvas.drawString("Shut down, wakes up at " + turnOnTimeStr, 45, 350);
	canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);

	delay(1000);

	tmElements_t tmOn;
	ezt::breakTime(turnOnTimeUTC, tmOn);

	M5.shutdown(RTC_Date(tmOn.Wday, tmOn.Month, tmOn.Day, tmOn.Year),
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

	WiFi.disconnect();

	while (WiFi.status() != WL_DISCONNECTED) {
		Serial.print(".");
		delay(100);
	}

	Serial.flush();

	esp_wifi_stop();

	// Light sleep and wait for timer or touch interrupt
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);  // TOUCH_INT
	esp_sleep_enable_timer_wakeup(sleepTime);
	esp_light_sleep_start();

	esp_wifi_start();

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
	if (isSetupMode)
		return;

	gui::loopGui();

	delay(100);

	while (millis() >= wakeUntilMillis) {
		if (shouldShutDown() && !utils::isCharging()) {
			shutDown();
		}

		sleep();
	}
}
