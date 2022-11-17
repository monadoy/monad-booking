#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <array>
#include <memory>

#include "utils.h"

// Add message names here, they must have corresponding keys in localization.json
#define L10N_MESSAGES(F)  \
	F(BOOK)               \
	F(FREE)               \
	F(CANCEL)             \
	F(UNTIL_NEXT)         \
	F(FREE_ROOM)          \
	F(BOOK_ROOM)          \
	F(NEXT_EVENT)         \
	F(NO_UPCOMING_EVENTS) \
	F(BOOKED)             \
	F(NOT_BOOKED)         \
	F(BOOK_ROOM_QUESTION) \
	F(FREE_CONFIRM_TITLE) \
	F(SETTINGS)           \
	F(VERSION)            \
	F(NEW_EVENT_SUMMARY)  \
	F(UPDATE)             \
	F(LATEST_VERSION)     \
	F(CHARGE_ME)

#define L10N_MESSAGE_AS_ENUM(M) M,
#define L10N_MESSAGE_AS_STRING(M) #M,

enum class L10nMessage { L10N_MESSAGES(L10N_MESSAGE_AS_ENUM) SIZE };

using L10nStringArray = std::array<String, (size_t)L10nMessage::SIZE>;

const L10nStringArray messageNames{L10N_MESSAGES(L10N_MESSAGE_AS_STRING)};

class Localization {
  public:
	std::unique_ptr<utils::Error> setLanguage(const String& lang);
	String msg(L10nMessage message);

  private:
	std::unique_ptr<utils::Error> _readMessages(const String& lang);

	String _currentLang = "";
	L10nStringArray _messages{};
};

#endif