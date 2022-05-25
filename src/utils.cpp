#include "utils.h"

#include <WiFi.h>
#include <WiFiClient.h>

namespace utils {
void connectWiFi(const String& ssid, const String& password) {
	ssid_ = ssid;
	password_ = password;

	auto startTime = millis();
	Serial.print(F("Connecting WiFi..."));

	WiFi.begin(ssid_.c_str(), password_.c_str());

	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(50);
	}

	Serial.println();
	Serial.print("WiFi connected in ");
	Serial.print(millis() - startTime);
	Serial.println(" ms");
}

void ensureWiFi() {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.print("Ensure WiFi: current status ");
		Serial.println(WiFi.status());
		connectWiFi(ssid_, password_);
	}
}

}  // namespace utils