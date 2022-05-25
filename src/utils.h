#include <Arduino.h>

namespace {
String ssid_;
String password_;
}  // namespace

namespace utils {
void connectWiFi(const String& ssid, const String& password);

void ensureWiFi();

}  // namespace utils