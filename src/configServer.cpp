#include "configServer.h"

#include <memory>

namespace Config {

void ConfigStore::loadConfigFromFlash(const String& fileName) {
	File configFileHandle;

	Serial.println("Trying to load config from flash...");

	configFileHandle = this->fs_.open(fileName, FILE_READ);
	if (configFileHandle) {
		auto err = deserializeMsgPack(this->config_, configFileHandle);
		if (err) {
			Serial.print("Cannot deserialize ");
			Serial.println(fileName);
		}
		Serial.print(fileName);
		Serial.println(" loaded from flash");
		return;
	}

	Serial.println("FALLBACK: Trying to load config.json from flash...");

	configFileHandle = fs_.open("/config.json", FILE_READ);
	if (configFileHandle) {
		auto err = deserializeJson(this->config_, configFileHandle);
		if (err) {
			Serial.println("Cannot deserialize config.json");
		}
		Serial.println("/config.json loaded from flash");
		return;
	}

	// Cannot read any configfile
	Serial.println("No existing config file found on flash");
};

Result<bool> ConfigStore::saveConfigToFlash(JsonVariantConst& newConfig) {
	File configFileHandle;

	configFileHandle = fs_.open(this->configFileName_, FILE_WRITE);

	if (!configFileHandle) {
		String errMsg = "Cannot open configfile for writing";
		Serial.println(errMsg);
		return Result<bool>::makeErr(
		    std::make_shared<ConfigError_t>(ConfigError_t{.errorMessage = errMsg}));
	}

	String configString = "";
	serializeMsgPack(newConfig, configString);
	configFileHandle.print(configString);

	return Result<bool>::makeOk(std::make_shared<bool>(true));
};

Result<String> ConfigStore::getTokenString() {
	String tokenString = "";
	serializeJson(this->config_["gcalsettings"]["token"], tokenString);
	if (tokenString.isEmpty()) {
		return Result<String>::makeErr(
		    std::make_shared<ConfigError_t>(ConfigError_t{.errorMessage = "Cannot get token"}));
	}
	return Result<String>::makeOk(std::make_shared<String>(tokenString));
};

bool ConfigServer::canHandle(AsyncWebServerRequest* request) {
	return request->url() == "/config" and request->method() == HTTP_GET;
};

void ConfigServer::handleRequest(AsyncWebServerRequest* request) {
	AsyncResponseStream* response = request->beginResponseStream("application/json");
	auto config = this->configStore_->getConfigJson();
	serializeJson(config, *response);
	request->send(response);
};

void ConfigServer::start() {
	this->server_ = new AsyncWebServer(this->port_);
	this->server_->addHandler(this);

	AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler(
	    "/config", [&](AsyncWebServerRequest* request, JsonVariant& configJson) {
		    // TODO: Probably good idea to validate the config before writing, somehow
		    Serial.println("Writing new config to filesystem...");

		    auto result = this->configStore_->saveConfigToFlash(configJson);
		    result.isOk() ? request->send(204)
		                  : request->send(500, "application/json",
		                                  "{\"error\":\"" + result.err()->errorMessage + "\"}");
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
