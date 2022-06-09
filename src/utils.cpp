#include "utils.h"

#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <ezTime.h>

#include <atomic>

const unsigned long TIMEOUT_S = 60;

bool isInSetupMode = false;
const IPAddress SETUP_IP_ADDR(192, 168, 69, 1);
const char* SETUP_SSID = "BOOKING_SETUP";
String passwordString = ("Monad" + utils::genRandomAppendix(3)).c_str();
const char* SETUP_PASS = passwordString.c_str();

String ssid_;
String password_;
bool disconnectRegistered = false;
std::atomic<bool> wifiSleep(false);

namespace utils {
void onDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
	Serial.println("WIFI Disconnected");
	if (!wifiSleep) {
		Serial.println("Reconnecting");
		WiFi.begin();
	}
}

void connectWiFi(const String& ssid, const String& password) {
	ssid_ = ssid;
	password_ = password;

	auto startTime = millis();
	Serial.print("Connecting WiFi...");

	if (wifiSleep) {
		wifiSleep = false;
		esp_wifi_start();
	}

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid_.c_str(), password_.c_str());

	if (!disconnectRegistered) {
		disconnectRegistered = true;
		WiFi.onEvent(onDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
	}

	unsigned long start = millis();

	while (!WiFi.isConnected() && millis() <= start + TIMEOUT_S * 1000) {
		delay(50);
		Serial.print(WiFi.status());
	}
	if (millis() > start + TIMEOUT_S * 1000) {
		Serial.println("WiFi connect timed out");
		return;
	}
	Serial.println();

	Serial.print("WiFi connected in ");
	Serial.print(millis() - startTime);
	Serial.println(" ms");
}

void ensureWiFi() {
	if (!WiFi.isConnected()) {
		Serial.print("Ensure WiFi: current status: ");
		Serial.println(WiFi.status());
		connectWiFi(ssid_, password_);
	}
}

void sleepWiFi() {
	if (!wifiSleep) {
		wifiSleep = true;
		esp_wifi_stop();
	}
}

bool isCharging() { return M5.getBatteryVoltage() > 4275; }

bool isAP() {
	wifi_mode_t mode = WiFi.getMode();
	return mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
}

String getApPassword() {
	if (isAP()) {
		wifi_config_t conf;
		esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
		String password = reinterpret_cast<const char*>(conf.ap.password);
		return password;
	}
	return "The M5Paper is not in AP mode.";
}

bool isSetupMode() { return isInSetupMode; }

void setupMode() {
	isInSetupMode = true;

	Serial.printf("Starting Access Point at \"%s\"\n", SETUP_SSID);
	Serial.printf("Wifi password is \"%s\"\n", SETUP_PASS);
	WiFi.disconnect();
	delay(100);
	WiFi.softAPConfig(SETUP_IP_ADDR, SETUP_IP_ADDR, IPAddress(255, 255, 255, 0));
	WiFi.softAP(SETUP_SSID, SETUP_PASS);
	WiFi.mode(WIFI_MODE_AP);
}

String genRandomAppendix(int length) {
	srand48(UTC.now());
	String appendix = "";
	for (int i = 0; i < length; i++) {
		appendix += String(rand() % 10);
	}
	return appendix;
}

std::vector<String> getBootLog() {
	fs::File readHandle = LittleFS.open("/boot.log", FILE_READ);
	if (!readHandle) {
		return std::vector<String>();
	}

	std::vector<String> entries{};
	String line;
	while (true) {
		line = readHandle.readStringUntil('\n');
		if (line.isEmpty())
			break;
		entries.push_back(std::move(line));
	}

	return entries;
}

bool addBootLogEntry(const String& entry) {
	auto entries = getBootLog();

	if (entries.size() > 11) {
		entries.erase(entries.begin());
	}

	entries.push_back(entry);

	fs::File writeHandle = LittleFS.open("/boot.log", FILE_WRITE);
	if (!writeHandle) {
		return false;
	}

	for (const String& entry : entries) {
		writeHandle.println(entry);
	}
	return true;
}

}  // namespace utils