#include "configServer.h"

#include <memory>

namespace config {

void ConfigStore::loadConfig() {
	// Release old memory if it exists
	config_ = DynamicJsonDocument(4096);

	log_i("Trying to load %s.json from flash...", configFileName_.c_str());

	File configFileHandle = fs_.open(configFileName_ + ".json", FILE_READ);
	if (configFileHandle) {
		auto err = deserializeJson(config_, configFileHandle);
		if (err) {
			log_e("Error deserializing %s.json: %s", configFileName_.c_str(), err.c_str());
		}
		log_i("%s.json loaded from flash", configFileName_.c_str());
		configFileHandle.close();
		return;
	}

	log_i("No existing config file found on flash");
};

Result<bool> ConfigStore::mergeConfig(JsonVariantConst newConfig) {
	File configFileHandle = fs_.open(configFileName_ + ".json", FILE_WRITE);

	if (!configFileHandle) {
		String errMsg = "Cannot open configfile for writing";
		log_e("%s", errMsg.c_str());
		return Result<bool>::makeErr(
		    std::make_shared<ConfigError_t>(ConfigError_t{.errorMessage = errMsg}));
	}

	DynamicJsonDocument copy = getConfigJsonCopy();

	utils::merge(copy.as<JsonVariant>(), newConfig);

	serializeJson(copy, configFileHandle);
	configFileHandle.close();

	return Result<bool>::makeOk(std::make_shared<bool>(true));
};

Result<bool> ConfigStore::remove() {
	config_.clear();
	auto err = fs_.remove(configFileName_);
	return !err ? Result<bool>::makeOk(std::make_shared<bool>(true))
	            : Result<bool>::makeErr(
	                new ConfigError_t{.errorMessage = "Could not remove config file from flash"});
};

bool ConfigServer::canHandle(AsyncWebServerRequest* request) {
	return request->url() == "/config" and request->method() == HTTP_GET;
};

void ConfigServer::handleRequest(AsyncWebServerRequest* request) {
	AsyncResponseStream* response = request->beginResponseStream("application/json");
	auto configDoc = configStore_->getConfigJsonCopy();
	auto config = configDoc.as<JsonVariant>();

	auto hideElement = [&](JsonVariant location, const char* key) {
		if (location && location.containsKey(key))
			location[key] = nullptr;
	};

	// Hide sensitive information
	hideElement(config["gcalsettings"], "token");
	hideElement(config["mscalsettings"], "token");
	hideElement(config["wifi"], "password");

	serializeJson(config, *response);
	request->send(response);
};

void ConfigServer::start() {
	server_ = new AsyncWebServer(port_);
	server_->addHandler(this);

	AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler(
	    "/config", [&](AsyncWebServerRequest* request, JsonVariant& configJson) {
		    // TODO: Probably good idea to validate the config before writing, somehow
		    log_i("Writing new config to filesystem...");

		    auto result = configStore_->mergeConfig(configJson);

		    result.isOk() ? request->send(204)
		                  : request->send(500, "application/json",
		                                  "{\"error\":\"" + result.err()->errorMessage + "\"}");
	    });

	server_->addHandler(configHandler);

	// Serve index.html without cache on the empty path
	server_->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/webroot/index.html");
	});

	// Serve assets with a large max-age to save bandwith.
	// They are fingerprinted to handle content updates.
	server_->serveStatic("/assets/", LittleFS, "/webroot/assets")
	    .setCacheControl("max-age=31536000");

	server_->onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "application/json", "{\"error\":\"Path does not exist\"}");
	});

	server_->begin();
};

}  // namespace config
