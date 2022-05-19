
#include <M5EPD.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "gui.h"

#define EZTIME_EZT_NAMESPACE 1
#include <ezTime.h>

#include "WebServer.h"

#define CONFIG_NAME "configuration"

// Uncomment this to load config variables from secrets.h
#define DEVMODE 1

// Provide official timezone names
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
const char* IANA_TZ = "Europe/Helsinki";
Timezone myTZ;

M5EPD_Canvas canvas(&M5.EPD);

const IPAddress SETUP_IP_ADDR(192, 168, 69, 1);
const char* SETUP_SSID = "BOOKING_SETUP";

// Initialize HTTP Server
WebServer webServer(80);

// Config store
Preferences preferences;

String WIFI_SSID = "";
String WIFI_PASS = "";

bool restoreWifiConfig();
void setupMode();
String makePage(String title, String contents);
void setRoutes();

struct tm timeinfo;

#ifdef DEVMODE

#include "secrets.h"
void loadSecrets(Preferences& prefs) {
	prefs.begin(CONFIG_NAME);

	prefs.putString("WIFI_SSID", SECRETS_WIFI_SSID);
	prefs.putString("WIFI_PASS", SECRETS_WIFI_PASS);
}
#endif

void connectWifi(const char* ssid, const char* pass) {
	Serial.print(F("Connecting WiFi..."));

	WiFi.begin(ssid, pass);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
}

void setupTime() {
	Serial.println("Setting up time");

	ezt::setDebug(INFO);
	ezt::waitForSync();

	myTZ.setLocation(IANA_TZ);
}

void printLocalTime() { Serial.println(myTZ.dateTime(RFC3339)); }

void setup() {
	M5.begin();

#ifdef DEVMODE
	loadSecrets(preferences);
#endif

	Serial.println(F("========== Monad Booking =========="));
	Serial.println(F("Booting up..."));
	preferences.begin(CONFIG_NAME);

	Serial.println(F("Setting up E-ink display..."));
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);

	Serial.println(F("Setting up RTC..."));
	M5.RTC.begin();

	Serial.println(F("Restoring WiFi-configuration..."));

	if (restoreWifiConfig()) {
		Serial.println(F("WiFi-config restored!"));
		connectWifi(WIFI_SSID.c_str(), WIFI_PASS.c_str());
		setupTime();
	} else {
		Serial.println(F("No wifi configuration stored"));
		Serial.println(F("Entering setup-mode..."));
		// Setupmode
		setupMode();
	}
	setRoutes();
	webServer.begin();
	initGui(&myTZ);
	ezt::events();
}

bool restoreWifiConfig() {
	delay(10);

	WIFI_SSID = preferences.getString("WIFI_SSID");
	WIFI_PASS = preferences.getString("WIFI_PASS");

	return WIFI_SSID.length() > 0;
}

String makePage(String title, String contents) {
	String s = "<!DOCTYPE html><html><head>";
	s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
	s += "<title>";
	s += title;
	s += "</title></head><body>";
	s += contents;
	s += "</body></html>";
	return s;
}

void setRoutes() {
	webServer.on("/", []() {
		Serial.println("Handling request for /");
		webServer.send(200, "text/html", makePage("TEST", "<h1>Toimiiko?</h1>"));
	});

	webServer.onNotFound([]() {
		String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
		Serial.println("Not found");
		webServer.send(200, "text/html", makePage("AP mode", s));
	});
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
	
}
