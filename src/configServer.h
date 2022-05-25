/*
 * Monad Booking Configuration handling
 */

#ifndef MONAD_BOOKING_CONFIGSERVER
#define MONAD_BOOKING_CONFIGSERVER

#include <ESPAsyncWebServer.h>

#include "ArduinoJson.h"
#include "AsyncJson.h"

namespace Config {

class ConfigStore {
  public:
	ConfigStore(fs::FS& fs, String configFileName = "/config.msgpack")
	    : fs_(fs), configFileName_(configFileName) {
		this->loadConfigFromFlash(configFileName);
	};
	JsonObjectConst getConfigJson();
	bool saveConfigToFlash(JsonVariant& newConfig);
	String getTokenString();

  protected:
	StaticJsonDocument<2048> config_;
	String configFileName_;
	bool loadConfigFromFlash(const String& fileName);
	fs::FS& fs_;
};

class ConfigServer : public AsyncWebHandler {
  public:
	/// Initialize the web server with the specified port
	explicit ConfigServer(uint16_t port);

	bool canHandle(AsyncWebServerRequest* request) override;
	void handleRequest(AsyncWebServerRequest* request) override;

	/// This is not a trivial handle
	bool isRequestHandlerTrivial() override final { return false; };

	/// Call this to start the server
	void start();

  protected:
	uint16_t port_;
	AsyncWebServer* server_ = nullptr;
	AsyncEventSource events_{"/events"};
};

}  // namespace Config

#endif