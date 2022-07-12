/*
 * Monad Booking Configuration handling
 */

#ifndef MONAD_BOOKING_CONFIGSERVER
#define MONAD_BOOKING_CONFIGSERVER

#define HTTP_DELETE WEBSERVER_HTTP_DELETE
#define HTTP_GET WEBSERVER_HTTP_GET
#define HTTP_HEAD WEBSERVER_HTTP_HEAD
#define HTTP_POST WEBSERVER_HTTP_POST
#define HTTP_PUT WEBSERVER_HTTP_PUT
#define HTTP_OPTIONS WEBSERVER_HTTP_OPTIONS
#define HTTP_PATCH WEBSERVER_HTTP_PATCH
#include <ESPAsyncWebServer.h>

#include "AsyncJson.h"
#undef HTTP_DELETE
#undef HTTP_GET
#undef HTTP_HEAD
#undef HTTP_POST
#undef HTTP_PUT
#undef HTTP_OPTIONS
#undef HTTP_PATCH

#include <LittleFS.h>

#include "ArduinoJson.h"
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
	    : fs_(fs), configFileName_(configFileName), config_{DynamicJsonDocument(2048)} {
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
	Result<bool> saveConfig(JsonVariantConst newConfig);
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

}  // namespace Config

#endif