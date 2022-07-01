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
		return utils::make_unique<utils::Error>("Could not open localization.json");
	}

	// Create filter to not waste memory on unused languages
	DynamicJsonDocument filter(1024 * 2);
	for (const String& name : messageNames) {
		filter[name][lang] = true;
	}

	auto err = deserializeJson(doc, handle, DeserializationOption::Filter(filter));
	if (err) {
		return utils::make_unique<utils::Error>(String("Error deserializing localization.json: ")
		                                        + err.c_str());
	}

	for (size_t msg = 0; msg < (size_t)L10nMessage::SIZE; msg++) {
		_messages[msg] = doc[messageNames[msg]][lang].as<String>();
		if (_messages[msg] == "") {
			return utils::make_unique<utils::Error>(String("Localization '") + messageNames[msg]
			                                        + "-" + lang + "' not found or empty");
		}
	}

	serializeJsonPretty(doc, Serial);

	return nullptr;
}

std::unique_ptr<utils::Error> Localization::setLanguage(const String& lang) {
	if (_currentLang == lang)
		return nullptr;

	_currentLang = lang;
	return _readMessages(_currentLang);
}

String Localization::msg(L10nMessage message) { return _messages[(size_t)message]; }
