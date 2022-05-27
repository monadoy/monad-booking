#include "utils.h"

#include <WiFi.h>
#include <WiFiClient.h>

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

}  // namespace utils