/*
 * Monad Booking Configuration handling
 */

#ifndef MONAD_BOOKING_CONFIGSERVER
#define MONAD_BOOKING_CONFIGSERVER

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "utils.h"

namespace config {

struct ConfigError_t {
	String errorMessage;
};

template <typename T>
using Result = utils::Result<T, ConfigError_t>;

/**
 * Configuration management
 *
 * Load, save and handle the configuration
 */
class ConfigStore {
  public:
	/**
	 * Load existing configuration from flash to memory, if any any
	 */
	ConfigStore(fs::FS& fs, String configFileName = "/config")
	    : fs_(fs), configFileName_(configFileName), config_{DynamicJsonDocument(4096)} {
		loadConfig();
	};
	/**
	 * Returns constant reference to the configuration object
	 */
	JsonObjectConst getConfigJson() { return config_.as<JsonObjectConst>(); };

	/**
	 * Returns a copy to the configuration object
	 */
	DynamicJsonDocument getConfigJsonCopy() { return config_; };

	/**
	 * Load configuration form flash and discard previously loaded config, if any
	 */
	void loadConfig();
	/**
	 * Merge new config with old if it exists and save to disk.
	 */
	Result<bool> mergeConfig(JsonVariantConst newConfig);
	/**
	 * Delete current configuration from memory and flash
	 */
	Result<bool> remove();

  protected:
	fs::FS& fs_;
	String configFileName_;
	DynamicJsonDocument config_;
};

class ConfigServer : public AsyncWebHandler {
  public:
	/// Initialize the web server with the specified port
	explicit ConfigServer(uint16_t port, ConfigStore* configStore)
	    : port_(port), configStore_(configStore){};

	bool canHandle(AsyncWebServerRequest* request) override;
	void handleRequest(AsyncWebServerRequest* request) override;

	/// This is not a trivial handle
	bool isRequestHandlerTrivial() override final { return false; };

	/// Call this to start the server
	void start();

  protected:
	uint16_t port_;
	ConfigStore* configStore_;
	AsyncWebServer* server_ = nullptr;
	AsyncEventSource events_{"/events"};
};

}  // namespace config

#endif