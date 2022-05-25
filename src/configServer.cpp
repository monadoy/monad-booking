#include "configServer.h"

#include <LittleFS.h>

namespace Config {

JsonObjectConst ConfigStore::getConfigJson() { return this->config_.as<JsonObjectConst>(); };

bool ConfigStore::loadConfigFromFlash(const String& fileName) {
	File configFileHandle;

	configFileHandle = this->fs_.open(fileName, FILE_READ);
	if (configFileHandle) {
		auto err = deserializeMsgPack(this->config_, configFileHandle);
		if (err) {
			Serial.println("Cannot deserialize config.msgpack");
			return false;
		}
		return true;
	}

	configFileHandle = fs_.open("config.json", FILE_READ);
	if (configFileHandle) {
		auto err = deserializeJson(this->config_, configFileHandle);
		if (err) {
			Serial.println("Cannot deserialize config.json");
			return false;
		}
		return true;
	}

	// Cannot read any configfile
	return false;
};

bool ConfigStore::saveConfigToFlash(JsonVariant& newConfig) {
	File configFileHandle;

	configFileHandle = fs_.open(this->configFileName_, FILE_WRITE);

	if (!configFileHandle) {
		Serial.println("Cannot open configfile for writing");
		return false;
	}

	String configString = "";
	serializeMsgPack(newConfig, configString);
	configFileHandle.print(configString);

	return true;
};

String ConfigStore::getTokenString() {
	String tokenString = "";
	serializeJson(this->config_["gcalsettings"]["token"], tokenString);
	return tokenString;
};

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

	this->server_->serveStatic("/", LittleFS, "/webroot/")
	    .setDefaultFile("index.html")
	    .setCacheControl("max-age=60");

	this->server_->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "application/json", "{\"error\":\"Path does not exist\"}");
	});

	this->server_->begin();
};

}  // namespace Config
