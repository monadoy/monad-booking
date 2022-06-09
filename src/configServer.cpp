#include "configServer.h"

#include <memory>

namespace Config {

void ConfigStore::loadConfig() {
	File configFileHandle;

	Serial.println("Trying to load config from flash...");

	// Release old memory if it exists
	config_.clear();

	configFileHandle = fs_.open(configFileName_ + ".msgpack", FILE_READ);
	if (configFileHandle) {
		auto err = deserializeMsgPack(config_, configFileHandle);
		if (err) {
			Serial.print("Cannot deserialize ");
			Serial.println(configFileName_ + ".msgpack");
		}
		Serial.print(configFileName_ + ".msgpack");
		Serial.println(" loaded from flash");
		return;
	}

	Serial.println("FALLBACK: Trying to load config.json from flash...");

	configFileHandle = fs_.open(configFileName_ + ".json", FILE_READ);
	if (configFileHandle) {
		auto err = deserializeJson(config_, configFileHandle);
		if (err) {
			Serial.println("Cannot deserialize " + configFileName_ + ".json");
		}
		Serial.println(configFileName_ + ".json loaded from flash");
		return;
	}

	// Cannot read any configfile
	Serial.println("No existing config file found on flash");
};

Result<bool> ConfigStore::saveConfig(JsonVariantConst newConfig) {
	File configFileHandle;

	configFileHandle = fs_.open(configFileName_ + ".msgpack", FILE_WRITE);

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

Result<bool> ConfigStore::remove() {
	config_.clear();
	auto err = fs_.remove(configFileName_);
	return !err ? Result<bool>::makeOk(std::make_shared<bool>(true))
	            : Result<bool>::makeErr(
	                new ConfigError_t{.errorMessage = "Could not remove config file from flash"});
};

Result<String> ConfigStore::getTokenString() {
	String tokenString = "";
	serializeJson(config_["gcalsettings"]["token"], tokenString);
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
	auto config = configStore_->getConfigJson();
	serializeJson(config, *response);
	request->send(response);
};

void ConfigServer::start() {
	server_ = new AsyncWebServer(port_);
	server_->addHandler(this);

	AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler(
	    "/config", [&](AsyncWebServerRequest* request, JsonVariant& configJson) {
		    // TODO: Probably good idea to validate the config before writing, somehow
		    Serial.println("Writing new config to filesystem...");

		    auto result = configStore_->saveConfig(configJson);

		    // Load config to keep internal state updated
		    configStore_->loadConfig();

		    result.isOk() ? request->send(204)
		                  : request->send(500, "application/json",
		                                  "{\"error\":\"" + result.err()->errorMessage + "\"}");
	    });

	server_->addHandler(configHandler);

	server_->serveStatic("/", LittleFS, "/webroot/")
	    .setDefaultFile("index.html")
	    .setCacheControl("max-age=60");

	server_->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "application/json", "{\"error\":\"Path does not exist\"}");
	});

	server_->begin();
};

}  // namespace Config
