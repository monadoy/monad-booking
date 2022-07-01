#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

#include <atomic>
#include <mutex>
#include <vector>

struct WiFiInfo {
	String ssid;
	String password;
	IPAddress ip;
};

/**
 * Designed as a singleton, creating multiple instances will probably mess stuff up.
 */
class WiFiManager {
  public:
	WiFiManager();

	/**
	 * Opposite of 'openAccessPoint()'.
	 * Starts wifi in STA mode.
	 * Basically connects the device to wifi.
	 * Returns true on successful connection, false on failure.
	 */
	bool openStation(const String& ssid, const String& password, int maxRetries = 0);

	/**
	 * Opposite of 'openStation()'.
	 * Starts wifi in AP mode.
	 * Opens an wifi access point that allows other devices to connect to it.
	 * Uses a randomly generated password and ssid.
	 */
	void openAccessPoint();

	/**
	 * Non-busy wait for wifi connection.
	 * Creates a new connection or waits for an existing one to complete.
	 */
	bool waitWiFi(int maxRetries = 0);

	/**
	 * Tells esp to start wifi and create a new connection.
	 */
	void wakeWiFi();

	/**
	 * Tells esp to stop wifi in order to keep wifi status in sync while sleeping.
	 */
	void sleepWiFi();

	bool isAccessPoint();

	WiFiInfo getAccessPointInfo();

	WiFiInfo getStationInfo();

	/*
	 * Protects variables below (_connectTimer, _connected, _disconnectReason),
	 * also blocks tasks in waitWiFi when connection is in progress.
	 * It is taken when starting connection and given when connection is done
	 * (either failed or succeeded).
	 */
	SemaphoreHandle_t _connectSemaphore = xSemaphoreCreateBinary();
	unsigned long _connectTimer = 0;
	wifi_err_reason_t _disconnectReason;
	String _disconnectReasonStr;

	/**
	 * Returns the reason for the last disconnection.
	 * If we are currently connected, will return the reason for last disconnect.
	 * Returns an empty string if we have yet to disconnect.
	 */
	String getDisconnectReason();

	/**
	 * These callbacks are called every time a disconnect happens.
	 */
	void registerErrorCallback(const std::function<void(String)>& callback) {
		_errorCallbacks.push_back(callback);
	};
	std::vector<std::function<void(String)>> _errorCallbacks;

  private:
	void _connect();
};

namespace {
const char* wifiErrorToString(wifi_err_reason_t errCode);
}
#endif
