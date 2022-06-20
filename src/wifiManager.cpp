#include "wifiManager.h"

#include <esp_wifi.h>

#include "globals.h"

const char* AP_SSID = "BOOKING-SETUP-";
const char* AP_PASS = "Monad-";

const IPAddress WiFiManager::AP_IP(192, 168, 1, 1);

// Make sure that we don't go to sleep while running an access point
void onAPEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
	switch (event) {
		case ARDUINO_EVENT_WIFI_AP_START:
			sleepManager.incrementTaskCounter();
		case ARDUINO_EVENT_WIFI_AP_STOP:
			sleepManager.decrementTaskCounter();
			break;
		default:
			log_e("Unhandled event");
			break;
	}
}

WiFiManager::WiFiManager() {
	WiFi.onEvent(onAPEvent, ARDUINO_EVENT_WIFI_AP_START);
	WiFi.onEvent(onAPEvent, ARDUINO_EVENT_WIFI_AP_STOP);
}

bool WiFiManager::openStation(const String& ssid, const String& password) {
	log_i("STA mode: connecting to AP...");

	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(true);
	WiFi.begin(ssid.c_str(), password.c_str());

	waitWiFi();

	// TODO: handle failure case
	return true;
}

void WiFiManager::waitWiFi() {
	auto startTime = millis();
	log_i("Waiting for WiFi...");
	while (!WiFi.isConnected()) {
		Serial.print(".");
		delay(20);
	}
	log_i("WiFi connected in %d ms.", millis() - startTime);
}

void WiFiManager::wakeWiFi() { esp_wifi_start(); }

void WiFiManager::sleepWiFi() { esp_wifi_stop(); }

void WiFiManager::openAccessPoint() {
	auto randNumString = [](size_t length) {
		String result{};
		for (size_t i = 0; i < length; i++) {
			uint32_t val = esp_random() % 10;
			result += val;
		}
		return result;
	};

	String ssid = String(AP_SSID) + randNumString(4);
	String pass = String(AP_PASS) + randNumString(4);

	log_i("Opening access point:\nssid: %s\npass: %s", ssid.c_str(), pass.c_str());

	WiFi.mode(WIFI_MODE_AP);
	WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(ssid.c_str(), pass.c_str());
}

bool WiFiManager::isAccessPoint() {
	wifi_mode_t mode = WiFi.getMode();
	return mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
}

APInfo WiFiManager::getAccessPointInfo() {
	wifi_config_t conf;
	esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
	const char* ssid = reinterpret_cast<const char*>(conf.ap.ssid);
	const char* password = reinterpret_cast<const char*>(conf.ap.password);
	return APInfo{.ssid = String{ssid}, .password = String{password}};
}

STAInfo WiFiManager::getStationInfo() {
	wifi_config_t conf;
	esp_wifi_get_config((wifi_interface_t)WIFI_IF_STA, &conf);
	const char* ssid = reinterpret_cast<const char*>(conf.sta.ssid);
	const char* password = reinterpret_cast<const char*>(conf.sta.password);
	return STAInfo{.ssid = String{ssid}, .password = String{password}, .ip = WiFi.localIP()};
}
