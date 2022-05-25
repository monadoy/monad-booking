/*
 * Monad Booking Configuration handling
 */

#ifndef MONAD_BOOKING_CONFIGSERVER
#define MONAD_BOOKING_CONFIGSERVER

#include <ESPAsyncWebServer.h>

namespace Config {

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