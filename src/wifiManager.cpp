#include "wifiManager.h"

#include <esp_wifi.h>

#include "globals.h"

const char* AP_SSID = "BOOKING-SETUP-";
const char* AP_PASS = "Monad-";

// Just make sure that we don't go to sleep while being an access point.
void onAPEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
	switch (event) {
		case ARDUINO_EVENT_WIFI_AP_START:
			log_i("AP_START");
			sleepManager.incrementTaskCounter();
			break;
		case ARDUINO_EVENT_WIFI_AP_STOP:
			log_i("AP_STOP");
			sleepManager.decrementTaskCounter();
			break;
		default:
			log_e("Unhandled event");
			break;
	}
}

/**
 * Give connection semaphores back when a connection attempt is completed.
 * Also dispatch error events when errors happen.
 */
void onSTAEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
	switch (event) {
		case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
			if (uxSemaphoreGetCount(wifiManager._connectSemaphore) == 0) {
				log_i("WiFi connected in %d ms", millis() - wifiManager._connectTimer);
				xSemaphoreGive(wifiManager._connectSemaphore);
			}
			break;
		}
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
			if (uxSemaphoreGetCount(wifiManager._connectSemaphore) == 0) {
				wifiManager._disconnectReason
				    = (wifi_err_reason_t)info.wifi_sta_disconnected.reason;
				wifiManager._disconnectReasonStr = wifiErrorToString(wifiManager._disconnectReason);

				// ASSOC_LEAVE seems to be one of the only non-error disconnect reasons.
				// All other reasons should be passed into error callbacks.
				if (wifiManager._disconnectReason != WIFI_REASON_ASSOC_LEAVE) {
					log_i("Wifi disconnect: %s", wifiManager._disconnectReasonStr.c_str());

					for (size_t i = 0; i < wifiManager._errorCallbacks.size(); i++)
						wifiManager._errorCallbacks[i](wifiManager._disconnectReasonStr);
				}

				xSemaphoreGive(wifiManager._connectSemaphore);
			}
			break;
		}
		default:
			log_e("Unhandled event");
			break;
	}
}

String WiFiManager::getDisconnectReasonString() {
	xSemaphoreTake(_connectSemaphore, portMAX_DELAY);
	String reason = wifiManager._disconnectReasonStr;
	xSemaphoreGive(_connectSemaphore);
	return reason;
}

wifi_err_reason_t WiFiManager::getDisconnectReason() {
	xSemaphoreTake(_connectSemaphore, portMAX_DELAY);
	wifi_err_reason_t reason = wifiManager._disconnectReason;
	xSemaphoreGive(_connectSemaphore);
	return reason;
}

WiFiManager::WiFiManager() {
	// Binary semaphore requires initialization
	xSemaphoreGive(_connectSemaphore);
	xSemaphoreGive(_waitWifiSemaphore);

	WiFi.onEvent(onAPEvent, ARDUINO_EVENT_WIFI_AP_START);
	WiFi.onEvent(onAPEvent, ARDUINO_EVENT_WIFI_AP_STOP);
	WiFi.onEvent(onSTAEvent, ARDUINO_EVENT_WIFI_STA_GOT_IP);
	WiFi.onEvent(onSTAEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

bool WiFiManager::openStation(const String& ssid, const String& password, int maxRetries) {
	log_i("Opening WiFi station...");
	_ssid = ssid;
	_password = password;

	WiFi.setAutoReconnect(false);
	return waitWiFi(maxRetries);
}

void WiFiManager::_connect() {
	xSemaphoreTake(_connectSemaphore, portMAX_DELAY);
	log_i("Connecting WiFi...");
	WiFi.begin(_ssid.c_str(), _password.c_str());
	_connectTimer = millis();
}

bool WiFiManager::waitWiFi(int maxRetries) {
	if (WiFi.isConnected()) {
		return true;
	}
	// If we're already waiting for a connection, just wait for it to complete.
	if (uxSemaphoreGetCount(_waitWifiSemaphore) == 0) {
		log_i("Waiting for existing connection attempt...");
		xSemaphoreTake(_waitWifiSemaphore, portMAX_DELAY);
		xSemaphoreGive(_waitWifiSemaphore);
		return WiFi.isConnected();
	}
	xSemaphoreTake(_waitWifiSemaphore, portMAX_DELAY);

	for (int tries = 0; tries <= maxRetries; tries++) {
		_connect();

		log_w("Waiting for connection, try #%d", tries + 1);

		// Wait for connection attempt to complete
		xSemaphoreTake(_connectSemaphore, portMAX_DELAY);
		xSemaphoreGive(_connectSemaphore);

		if (WiFi.isConnected()) {
			xSemaphoreGive(_waitWifiSemaphore);
			return true;
		} else {
			esp_wifi_deauth_sta(0);
		}
	}
	log_w("Couldn't connect to WiFi even after %d tries.", maxRetries + 1);
	xSemaphoreGive(_waitWifiSemaphore);
	return false;
}

void WiFiManager::wakeWiFi() {
	esp_wifi_start();
	log_i("WiFi status after start: %d", WiFi.status());
}

void WiFiManager::sleepWiFi() {
	esp_wifi_deauth_sta(0);
	esp_wifi_stop();
}

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

	log_i("Opening access point");

	WiFi.softAP(ssid.c_str(), pass.c_str());
}

bool WiFiManager::isAccessPoint() {
	wifi_mode_t mode = WiFi.getMode();
	return mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
}

WiFiInfo WiFiManager::getAccessPointInfo() {
	wifi_config_t conf;
	esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
	const char* ssid = reinterpret_cast<const char*>(conf.ap.ssid);
	const char* password = reinterpret_cast<const char*>(conf.ap.password);
	return WiFiInfo{.ssid = String{ssid}, .password = String{password}, .ip = WiFi.softAPIP()};
}

WiFiInfo WiFiManager::getStationInfo() {
	wifi_config_t conf;
	esp_wifi_get_config((wifi_interface_t)WIFI_IF_STA, &conf);
	const char* ssid = reinterpret_cast<const char*>(conf.sta.ssid);
	const char* password = reinterpret_cast<const char*>(conf.sta.password);
	return WiFiInfo{.ssid = String{ssid}, .password = String{password}, .ip = WiFi.localIP()};
}

namespace {
const char* wifiErrorToString(wifi_err_reason_t errCode) {
	switch ((wifi_err_reason_t)errCode) {
#if CORE_DEBUG_LEVEL > 2
		case WIFI_REASON_UNSPECIFIED:
			return "UNSPECIFIED";
		case WIFI_REASON_AUTH_EXPIRE:
			return "AUTH_EXPIRE";
		case WIFI_REASON_AUTH_LEAVE:
			return "AUTH_LEAVE";
		case WIFI_REASON_ASSOC_EXPIRE:
			return "ASSOC_EXPIRE";
		case WIFI_REASON_ASSOC_TOOMANY:
			return "ASSOC_TOOMANY";
		case WIFI_REASON_NOT_AUTHED:
			return "NOT_AUTHED";
		case WIFI_REASON_NOT_ASSOCED:
			return "NOT_ASSOCED";
		case WIFI_REASON_ASSOC_LEAVE:
			return "ASSOC_LEAVE";
		case WIFI_REASON_ASSOC_NOT_AUTHED:
			return "ASSOC_NOT_AUTHED";
		case WIFI_REASON_DISASSOC_PWRCAP_BAD:
			return "DISASSOC_PWRCAP_BAD";
		case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
			return "DISASSOC_SUPCHAN_BAD";
		case WIFI_REASON_IE_INVALID:
			return "IE_INVALID";
		case WIFI_REASON_MIC_FAILURE:
			return "MIC_FAILURE";
		case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
			return "4WAY_HANDSHAKE_TIMEOUT";
		case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
			return "GROUP_KEY_UPDATE_TIMEOUT";
		case WIFI_REASON_IE_IN_4WAY_DIFFERS:
			return "IE_IN_4WAY_DIFFERS";
		case WIFI_REASON_GROUP_CIPHER_INVALID:
			return "GROUP_CIPHER_INVALID";
		case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
			return "PAIRWISE_CIPHER_INVALID";
		case WIFI_REASON_AKMP_INVALID:
			return "AKMP_INVALID";
		case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
			return "UNSUPP_RSN_IE_VERSION";
		case WIFI_REASON_INVALID_RSN_IE_CAP:
			return "INVALID_RSN_IE_CAP";
		case WIFI_REASON_802_1X_AUTH_FAILED:
			return "802_1X_AUTH_FAILED";
		case WIFI_REASON_CIPHER_SUITE_REJECTED:
			return "CIPHER_SUITE_REJECTED";
		case WIFI_REASON_BEACON_TIMEOUT:
			return "BEACON_TIMEOUT";
		case WIFI_REASON_NO_AP_FOUND:
			return "NO_AP_FOUND";
		case WIFI_REASON_AUTH_FAIL:
			return "AUTH_FAIL";
		case WIFI_REASON_ASSOC_FAIL:
			return "ASSOC_FAIL";
		case WIFI_REASON_HANDSHAKE_TIMEOUT:
			return "HANDSHAKE_TIMEOUT";
#endif
		default:
			return "Unknown WiFi error";
	}
}
}  // namespace