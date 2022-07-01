#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <array>
#include <memory>

#include "utils.h"

enum class L10nMessage {
	BOOT_WIFI_FAIL,
	BOOT_NTP_FAIL,
	BOOTING,
	BOOK,
	FREE,
	CANCEL,
	UNTIL_NEXT,
	FREE_ROOM,
	BOOK_ROOM,
	NEXT_EVENT,
	NO_UPCOMING_EVENTS,
	BOOKED,
	BOOK_ROOM_QUESTION,
	RELEASE_QUESTION,
	SIZE
};

using L10nMessageArray = std::array<String, (size_t)L10nMessage::SIZE>;

const L10nMessageArray messageNames{"BOOT_WIFI_FAIL",
                                    "BOOT_NTP_FAIL",
                                    "BOOTING",
                                    "BOOK",
                                    "FREE",
                                    "CANCEL",
                                    "UNTIL_NEXT",
                                    "FREE_ROOM",
                                    "BOOK_ROOM",
                                    "NEXT_EVENT",
                                    "NO_UPCOMING_EVENTS",
                                    "BOOKED",
                                    "BOOK_ROOM_QUESTION",
                                    "RELEASE_QUESTION"};

class Localization {
  public:
	std::unique_ptr<utils::Error> setLanguage(const String& lang);
	String msg(L10nMessage message);

  private:
	std::unique_ptr<utils::Error> _readMessages(const String& lang);

	String _currentLang = "";
	L10nMessageArray _messages{};
};

#endif