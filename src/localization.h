#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <array>
#include <memory>

#include "utils.h"

namespace loc {

enum class Message {
	BOOT_WIFI_FAIL,
	BOOT_NTP_FAIL,
	BOOTING,
	RESERVE,
	FREE,
	CANCEL,
	UNTIL_NEXT,
	SIZE
};

std::unique_ptr<utils::Error> setLanguage(const String& lang);

const char* const& getMessage(Message message);

}  // namespace loc

#endif