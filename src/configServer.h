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

namespace Config {

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
	    : fs_(fs), configFileName_(configFileName) {
		loadConfig();
	};
	/**
	 * Returns constant reference to the configuration object
	 */
	JsonObjectConst getConfigJson() { return config_.as<JsonObjectConst>(); };
	/**
	 * Load configuration form flash and discard previously loaded config, if any
	 */
	void loadConfig();
	/**
	 * Save configuration to flash and discard previously loaded config, if any
	 */
	Result<bool> saveConfig(JsonVariantConst newConfig);
	/**
	 * Return contents of the token.json as string
	 */
	Result<String> getTokenString();
	/**
	 * Delete current configuration from memory and flash
	 */
	Result<bool> remove();

  protected:
	fs::FS& fs_;
	String configFileName_;
	StaticJsonDocument<2048> config_;
};

class ConfigServer : public AsyncWebHandler {
  public:
	/// Initialize the web server with the specified port
	explicit ConfigServer(uint16_t port, std::shared_ptr<ConfigStore> configStore)
	    : port_(port), configStore_(configStore){};

	bool canHandle(AsyncWebServerRequest* request) override;
	void handleRequest(AsyncWebServerRequest* request) override;

	/// This is not a trivial handle
	bool isRequestHandlerTrivial() override final { return false; };

	/// Call this to start the server
	void start();

  protected:
	uint16_t port_;
	std::shared_ptr<ConfigStore> configStore_;
	AsyncWebServer* server_ = nullptr;
	AsyncEventSource events_{"/events"};
};

}  // namespace Config

#endif