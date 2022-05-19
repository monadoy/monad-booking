#include "configServer.h"

#include <LittleFS.h>

#include "ArduinoJson.h"
#include "AsyncJson.h"

namespace Config {

ConfigServer::ConfigServer(uint16_t port) : port_(port) {}

bool ConfigServer::canHandle(AsyncWebServerRequest* request) {
	return request->url() == "/config" and request->method() == HTTP_GET;
};

void ConfigServer::handleRequest(AsyncWebServerRequest* request) {
	AsyncWebServerResponse* response
	    = request->beginResponse(LittleFS, "/config.json", "application/json");
	request->send(response);
};

void ConfigServer::start() {
	this->server_ = new AsyncWebServer(this->port_);
	this->server_->addHandler(this);

	AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler(
	    "/config", [](AsyncWebServerRequest* request, JsonVariant& json) {
		    // TODO: Probably good idea to validate the config before writing, somehow
		    Serial.println("Writing new config to filesystem...");

		    File configFile = LittleFS.open("/config.json", FILE_WRITE);

		    if (!configFile) {
			    Serial.println("Cannot write to filesystem...");
		    } else {
			    String configString = "";
			    serializeJson(json, configString);
			    configFile.print(configString);
			    request->send(204);
		    }
	    });

	this->server_->addHandler(configHandler);

	this->server_->serveStatic("/", LittleFS, "/")
	    .setDefaultFile("index.html")
	    .setCacheControl("max-age=60");

	this->server_->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "application/json", "{\"error\":\"Path does not exist\"}");
	});

	this->server_->begin();
};

}  // namespace Config
