
#include <Arduino.h>
#include <LittleFS.h>
#include <M5EPD.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>

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

#define MICROS_PER_SEC 1000000
#define MILLIS_PER_SEC 1000
const uint64_t LIGHT_SLEEP_TIME = 120 * MICROS_PER_SEC;

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

void loop() {
	if (!isSetupMode) {
		Serial.print("loop at ");
		Serial.println(millis());

		loopGui();

		Serial.flush();

		// Light sleep and wait for timer or touch interrupt to continue looping
		esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);  // TOUCH_INT
		esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_TIME);
		esp_light_sleep_start();
	}
}
