#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

#include <atomic>
#include <mutex>

struct APInfo {
	String ssid;
	String password;
};

struct STAInfo {
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
	 * Use when connecting to an access point.
	 * Returns true on successful connection, false on failure.
	 */
	bool openStation(const String& ssid, const String& password);

	bool waitWiFi();

	void wakeWiFi();

	void sleepWiFi();

	/**
	 * Open an access point to allow other devices to connect to it.
	 * Uses a randomly generated password and ssid.
	 */
	void openAccessPoint();

	bool isAccessPoint();

	APInfo getAccessPointInfo();

	STAInfo getStationInfo();

	static const IPAddress AP_IP;

	std::atomic<unsigned long> _connectTimer;

  private:
	void _connect();

	std::mutex _waitWiFiMutex;
	std::atomic<bool> _connecting{false};
};

#endif
