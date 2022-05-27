
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>

#include "gui.h"
#include "utils.h"

#define EZTIME_EZT_NAMESPACE 1
#include <ezTime.h>

#include "configServer.h"

// Format the filesystem automatically if not formatted already
#define FORMAT_LITTLEFS_IF_FAILED true

#define EZTIME_EZT_NAMESPACE 1

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

String WIFI_SSID = "";
String WIFI_PASS = "";

bool isSetupMode = false;

#define MICROS_PER_MILLI 1000
#define MILLIS_PER_SEC 1000
#define MICROS_PER_SEC 1000000
const uint64_t UPDATE_STATUS_INTERVAL_MS = 120 * MILLIS_PER_SEC;
const uint64_t WAKE_TIME_MS = 400;

bool restoreWifiConfig();
void setupMode();

struct tm timeinfo;

#ifdef DEVMODE

#include "secrets.h"
void loadSecrets(Preferences& prefs) {
	prefs.begin(CONFIG_NAME);

	prefs.putString("WIFI_SSID", SECRETS_WIFI_SSID);
	prefs.putString("WIFI_PASS", SECRETS_WIFI_PASS);
}
#endif

void setupTime() {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);
	ezt::waitForSync();

	myTZ.setLocation(IANA_TZ);
}

void printLocalTime() { Serial.println(myTZ.dateTime(RFC3339)); }

void setup() {
#ifdef USE_EXTERNAL_SERIAL
	Serial.begin(115200, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_19);
	delay(100);
#endif

	M5.begin(true, false, !USE_EXTERNAL_SERIAL, true, true);

#ifdef DEVMODE
	loadSecrets(preferences);
#endif

	Serial.println(F("========== Monad Booking =========="));
	Serial.println(F("Booting up..."));
	preferences.begin(CONFIG_NAME);

	Serial.println(F("Setting up LittleFS..."));
	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
		Serial.println("LittleFS Mount Failed");
		Serial.println("Please ensure your partition layout has spiffs partition defined");
		return;
	}

	Serial.println(F("Setting up E-ink display..."));
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);

	Serial.println(F("Setting up RTC..."));
	M5.RTC.begin();

	Serial.println(F("Restoring WiFi-configuration..."));

	if (restoreWifiConfig()) {
		Serial.println(F("WiFi-config restored!"));
		utils::connectWiFi(WIFI_SSID, WIFI_PASS);
		setupTime();
		initGui(&myTZ);
		delay(3000);
	} else {
		isSetupMode = true;
		Serial.println(F("No wifi configuration stored"));
		Serial.println(F("Entering setup-mode..."));
		// Setupmode
		setupMode();
		// Initialize Configserver
		// TODO: this is currently thrown away after setup() ends
		Config::ConfigServer* configServer = new Config::ConfigServer(80);

		configServer->start();
	}
}
bool restoreWifiConfig() {
	delay(10);

	WIFI_SSID = preferences.getString("WIFI_SSID");
	WIFI_PASS = preferences.getString("WIFI_PASS");

	return WIFI_SSID.length() > 0;
}

void setupMode() {
	Serial.printf("Starting Access Point at \"%s\"\n", SETUP_SSID);
	WiFi.disconnect();
	delay(100);
	WiFi.softAPConfig(SETUP_IP_ADDR, SETUP_IP_ADDR, IPAddress(255, 255, 255, 0));
	WiFi.softAP(SETUP_SSID);
	WiFi.mode(WIFI_MODE_AP);
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
		updateGui();

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

	loopGui();

	delay(100);

	while (millis() >= wakeUntilMillis) {
		sleep();
	}
}
