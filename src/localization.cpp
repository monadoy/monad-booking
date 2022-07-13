#include "localization.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "utils.h"

/**
 * Read localized messages from file and insert them into global "messages" as a side effect.
 * Returns an error or nullptr.
 */
std::unique_ptr<utils::Error> Localization::_readMessages(const String& lang) {
	log_i("Trying to load localization.json from flash...");
	DynamicJsonDocument doc(1024 * 16);
	File handle = LittleFS.open("/localization.json", FILE_READ);

	if (!handle) {
		return utils::make_unique<utils::Error>("Could not open localization.json.");
	}

	// Create filter to not waste memory on unused languages
	DynamicJsonDocument filter(1024 * 2);
	filter["supportedLanguages"] = true;
	for (const String& name : messageNames) {
		filter[name][lang] = true;
	}

	auto err = deserializeJson(doc, handle, DeserializationOption::Filter(filter));
	if (err) {
		return utils::make_unique<utils::Error>(String("Error deserializing localization.json: ")
		                                        + err.c_str() + ".");
	}

	bool isSupported = false;
	for (JsonVariantConst obj : doc["supportedLanguages"].as<JsonArrayConst>()) {
		if (lang == obj.as<String>()) {
			isSupported = true;
		}
	}
	if (!isSupported)
		return utils::make_unique<utils::Error>("localization.json: language '" + lang
		                                        + "' not supported.");

	for (size_t msg = 0; msg < (size_t)L10nMessage::SIZE; msg++) {
		_messages[msg] = doc[messageNames[msg]][lang].as<String>();
		if (_messages[msg] == "null") {
			return utils::make_unique<utils::Error>(String("Localization '") + lang + "-"
			                                        + messageNames[msg] + "' not found.");
		}
	}

#if CORE_DEBUG_LEVEL > 3
	serializeJsonPretty(doc, Serial);
#endif

	return nullptr;
}

std::unique_ptr<utils::Error> Localization::setLanguage(const String& lang) {
	if (lang == "null") {
		return utils::make_unique<utils::Error>("'language' key not set in config.json.");
	}

	if (_currentLang == lang)
		return nullptr;

	_currentLang = lang;
	return _readMessages(_currentLang);
}

String Localization::msg(L10nMessage message) { return _messages[(size_t)message]; }
