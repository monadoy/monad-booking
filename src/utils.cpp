#include "utils.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>

const unsigned long TIMEOUT_S = 20;

namespace utils {
void connectWiFi(const String& ssid, const String& password) {
	ssid_ = ssid;
	password_ = password;

	auto startTime = millis();
	Serial.print("Connecting WiFi...");

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid_.c_str(), password_.c_str());
	WiFi.setAutoReconnect(false);

	unsigned long start = millis();

	while (!WiFi.isConnected() && millis() <= start + TIMEOUT_S * 1000) {
		delay(100);
		Serial.print(".");
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

bool isCharging() { return M5.getBatteryVoltage() > 4275; }

bool isAP() {
	wifi_mode_t mode = WiFi.getMode();
	if(mode==WIFI_MODE_AP || mode==WIFI_MODE_APSTA){
		return true;
	}
	return false;
}

String getApPassword() {
	if(isAP()) {
		wifi_config_t conf;
    	esp_wifi_get_config((wifi_interface_t)WIFI_IF_STA, &conf);
		String password = reinterpret_cast<const char*>(conf.sta.password);
		return password;
	}
	return "The M5Paper is not in AP mode.";
}

}  // namespace utils