#include "localization.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "utils.h"

namespace loc {

const std::array<const char*, (size_t)Language::SIZE> languageCodes = {"NONE", "FI", "EN"};

const std::array<const char*, (size_t)Message::SIZE> messageNames
    = {"BOOT_WIFI_FAIL", "BOOT_NTP_FAIL", "BOOTING", "RESERVE", "FREE", "CANCEL", "UNTIL_NEXT"};

Language currentLang = Language::NONE;

using MessagesArray = std::array<const char*, (size_t)Message::SIZE>;

MessagesArray messages{};

/**
 * Read localized messages from file and insert them into global "messages" as a side effect.
 * Returns an error or nullptr.
 */
std::unique_ptr<utils::Error> readMessages(Language lang) {
	log_i("Trying to load localization.json from flash...");
	DynamicJsonDocument doc(1024 * 16);
	File handle = LittleFS.open("/localization.json", FILE_READ);

	if (!handle) {
		return utils::make_unique<utils::Error>("Could not open localization.json");
	}

	const char* langCode = languageCodes[(size_t)lang];

	// Create filter to not waste memory on unused languages
	DynamicJsonDocument filter(1024 * 2);
	for (auto name : messageNames) {
		filter[name][langCode] = true;
	}

	auto err = deserializeJson(doc, handle, DeserializationOption::Filter(filter));
	if (err) {
		return utils::make_unique<utils::Error>(String("Error deserializing localization.json: ")
		                                        + err.c_str());
	}

	for (size_t msg = 0; msg < (size_t)Message::SIZE; msg++) {
		messages[msg] = doc[messageNames[msg]][langCode];
		if (messages[msg] == "") {
			return utils::make_unique<utils::Error>(String("Localization key '") + messageNames[msg]
			                                        + "' not found or empty");
		}
	}

	serializeJsonPretty(doc, Serial);

	return nullptr;
}

std::unique_ptr<utils::Error> setLanguage(String langCode) {
	Language lang = Language::NONE;
	for (size_t i = 0; i < (size_t)Language::SIZE; i++) {
		if (langCode == languageCodes[i]) {
			lang = (Language)i;
		}
	}

	if (lang == Language::NONE)
		return utils::make_unique<utils::Error>("Invalid language code in config: '" + langCode
		                                        + "'");

	if (currentLang == lang)
		return nullptr;

	currentLang = lang;
	return readMessages(currentLang);
}

const char* const& getMessage(Message message) { return messages[(size_t)message]; }
}  // namespace loc