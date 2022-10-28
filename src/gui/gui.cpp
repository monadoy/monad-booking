#include "gui.h"

#include "LittleFS.h"
#include "globals.h"

namespace gui {

// FONT SIZES
const uint8_t FS_BOOTLOG = 16;
const uint8_t FS_NORMAL = 24;
const uint8_t FS_BUTTON = 28;
const uint8_t FS_HEADER = 32;
const uint8_t FS_TIMESPAN = 44;
const uint8_t FS_TITLE = 64;
const uint8_t BK = 15;  // Black
const uint8_t WH = 0;   // White

// Panel colors
const uint8_t L_PNL = 0;
const uint8_t L_PNL_TAKEN = 15;
const uint8_t R_PNL = 1;
const uint8_t R_PNL_TAKEN = 3;

const char* FONT_BOLD = "/interbold.ttf";
const char* FONT_REGULAR = "/interregular.ttf";

#define PNL(s, p) _screens[size_t(s)].panels[size_t(p)]
#define TXT(s, t) _screens[size_t(s)].texts[size_t(t)]
#define BTN(s, b) _screens[size_t(s)].buttons[size_t(b)]

#define ADD_PNL(s, p, pnl)                                  \
	assert(_screens[size_t(s)].panels.size() == size_t(p)); \
	_screens[size_t(s)].panels.push_back(utils::make_unique<Panel> pnl)

#define ADD_TXT(s, t, txt)                                 \
	assert(_screens[size_t(s)].texts.size() == size_t(t)); \
	_screens[size_t(s)].texts.push_back(utils::make_unique<Text> txt)

#define ADD_BTN(s, b, btn)                                   \
	assert(_screens[size_t(s)].buttons.size() == size_t(b)); \
	_screens[size_t(s)].buttons.push_back(utils::make_unique<Button> btn)

GUI::GUI() {
	M5EPD_Canvas font(&M5.EPD);
	font.setTextFont(1);
	font.loadFont(FONT_BOLD, LittleFS);
	font.createRender(FS_BUTTON, 64);
	font.createRender(FS_TITLE, 128);
	font.createRender(FS_HEADER, 64);

	font.setTextFont(2);
	font.loadFont(FONT_REGULAR, LittleFS);
	font.createRender(FS_NORMAL, 64);
	font.createRender(FS_HEADER, 64);
	font.createRender(FS_TIMESPAN, 128);
	font.createRender(FS_BOOTLOG, 64);

	// Initialize screens (only loading screen for now)
	// NOTICE: MAKE SURE THAT THE ENUMS ARE IN SYNC WITH THE ARRAYS

	// Loading screen init
	// TODO
}

void GUI::initMain(cal::Model* model) {
	_model = model;

	// Initialize the rest of the screens (loading was initialized in constructor)
	// NOTICE: MAKE SURE THAT THE ENUMS ARE IN SYNC WITH THE ARRAYS

	// MAIN SCREEN PANELS
	ADD_PNL(ScreenId::MAIN, MainPanelId::LEFT, (Pos{0, 0}, Size{652, 540}, L_PNL));
	ADD_PNL(ScreenId::MAIN, MainPanelId::RIGHT, (Pos{653, 0}, Size{308, 540}, R_PNL));

	// MAIN SCREEN TEXT
	ADD_TXT(ScreenId::MAIN, MainTextId::TOP_CLOCK,
	        (Pos{875, 16}, Size{77, 40}, "00:00", FS_NORMAL, BK, R_PNL));
	ADD_TXT(ScreenId::MAIN, MainTextId::MID_CLOCK,
	        (Pos{80, 92}, Size{77, 40}, "00:00", FS_NORMAL, BK, L_PNL));
	ADD_TXT(ScreenId::MAIN, MainTextId::ROOM_NAME,
	        (Pos{80, 125}, Size{652 - 90, 40}, "room", FS_NORMAL, BK, L_PNL));
	ADD_TXT(ScreenId::MAIN, MainTextId::HEADER,
	        (Pos{80, 164}, Size{418, 77}, l10n.msg(L10nMessage::NOT_BOOKED), FS_TITLE, BK, L_PNL,
	         true));

	ADD_TXT(ScreenId::MAIN, MainTextId::L_FREE_SUBHEADER,
	        (Pos{80, 249}, Size{300, 60}, l10n.msg(L10nMessage::BOOK_ROOM), FS_HEADER, BK, L_PNL,
	         true));

	ADD_TXT(ScreenId::MAIN, MainTextId::L_TAKEN_ORGANIZER,
	        (Pos{80, 252}, Size{652 - 90, 40}, "organizer@gmail.com", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(ScreenId::MAIN, MainTextId::L_TAKEN_SUMMARY,
	        (Pos{80, 289}, Size{652 - 90, 40}, "summary", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(ScreenId::MAIN, MainTextId::L_TAKEN_TIMESPAN,
	        (Pos{80, 330}, Size{412, 53}, "00:00 - 00:00", FS_TIMESPAN, WH, L_PNL_TAKEN, true));

	ADD_TXT(ScreenId::MAIN, MainTextId::R_FREE_HEADER,
	        (Pos{701, 359}, Size{231, 135}, l10n.msg(L10nMessage::NO_UPCOMING_EVENTS), FS_HEADER,
	         BK, R_PNL, true));

	ADD_TXT(ScreenId::MAIN, MainTextId::R_TAKEN_HEADER,
	        (Pos{701, 161}, Size{231, 135}, l10n.msg(L10nMessage::NEXT_EVENT), FS_HEADER, BK,
	         R_PNL_TAKEN, true));
	ADD_TXT(ScreenId::MAIN, MainTextId::R_TAKEN_ORGANIZER,
	        (Pos{701, 246}, Size{239, 40}, "organizer@gmail.com", FS_NORMAL, BK, R_PNL_TAKEN));
	ADD_TXT(ScreenId::MAIN, MainTextId::R_TAKEN_SUMMARY,
	        (Pos{701, 279}, Size{231, 87}, "summary", FS_NORMAL, BK, R_PNL_TAKEN));

	ADD_TXT(ScreenId::MAIN, MainTextId::R_TAKEN_TIMESPAN,
	        (Pos{701, 370}, Size{231, 106}, "00:00 -\n00:00", FS_TIMESPAN, BK, R_PNL_TAKEN, true));

	// MAIN SCREEN BUTTONS
	// TODO
}

void GUI::setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status) {
	if (status) {
		_status = status;
		TXT(ScreenId::MAIN, MainTextId::ROOM_NAME)->setText(_status->name);
		if (_status->currentEvent) {
			auto ce = _status->currentEvent;
			TXT(ScreenId::MAIN, MainTextId::L_TAKEN_ORGANIZER)->setText(ce->creator);
			TXT(ScreenId::MAIN, MainTextId::L_TAKEN_SUMMARY)->setText(ce->summary);
			TXT(ScreenId::MAIN, MainTextId::L_TAKEN_TIMESPAN)
			    ->setText(safeMyTZ.dateTime(ce->unixStartTime, UTC_TIME, "G:i") + " - "
			              + safeMyTZ.dateTime(ce->unixEndTime, UTC_TIME, "G:i"));
		}
		if (_status->nextEvent) {
			auto ne = _status->nextEvent;
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_ORGANIZER)->setText(ne->creator);
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_SUMMARY)->setText(ne->summary);
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_TIMESPAN)
			    ->setText(safeMyTZ.dateTime(ne->unixStartTime, UTC_TIME, "G:i") + " -\n"
			              + safeMyTZ.dateTime(ne->unixEndTime, UTC_TIME, "G:i"));
		}
	};

	TXT(ScreenId::MAIN, MainTextId::TOP_CLOCK)->setText(safeMyTZ.dateTime("G:i"));
	TXT(ScreenId::MAIN, MainTextId::MID_CLOCK)->setText(safeMyTZ.dateTime("G:i"));

	// Do a full draw on main screen only if status has changed
	if (_currentScreen == ScreenId::MAIN) {
		if (status)
			draw();
		else
			draw();  // TODO: replace full draw with a lightweight draw (only clock and battery
			         // level with less flashing)
	}

	// Always draw when coming from a confirmation screen, (or loading screen)
	if (_currentScreen == ScreenId::CONFIRM_FREE || _currentScreen == ScreenId::LOADING) {
		switchToScreen(ScreenId::MAIN);
		draw();
	}
}

void GUI::handleTouch(int16_t x, int16_t y) {
	// for (auto& button : _screens[size_t(_currentScreen)].buttons) {
	// 	button.handleTouch(x, y);
	// }
}

void GUI::wake() { M5.EPD.Active(); }

void GUI::sleep() {
	if (_currentScreen != ScreenId::MAIN) {
		switchToScreen(ScreenId::MAIN);
	}

	M5.EPD.Sleep();
}

void GUI::switchToScreen(ScreenId screenId) {
	log_i("Switching to screen %d", size_t(_currentScreen));

	for (auto& panel : _screens[size_t(_currentScreen)].panels) panel->hide();
	for (auto& text : _screens[size_t(_currentScreen)].texts) text->hide();
	for (auto& button : _screens[size_t(_currentScreen)].buttons) button->hide();

	_currentScreen = screenId;

	for (auto& panel : _screens[size_t(_currentScreen)].panels) panel->show();
	for (auto& text : _screens[size_t(_currentScreen)].texts) text->show();
	for (auto& button : _screens[size_t(_currentScreen)].buttons) button->show();

	// Hide specific things on main screen based on taken or free
	if (_currentScreen == ScreenId::MAIN) {
		if (_status->currentEvent) {
			TXT(ScreenId::MAIN, MainTextId::L_FREE_SUBHEADER)->hide();
		} else {
			TXT(ScreenId::MAIN, MainTextId::L_TAKEN_ORGANIZER)->hide();
			TXT(ScreenId::MAIN, MainTextId::L_TAKEN_SUMMARY)->hide();
			TXT(ScreenId::MAIN, MainTextId::L_TAKEN_TIMESPAN)->hide();
		}

		if (_status->nextEvent) {
			TXT(ScreenId::MAIN, MainTextId::R_FREE_HEADER)->hide();
		} else {
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_HEADER)->hide();
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_ORGANIZER)->hide();
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_SUMMARY)->hide();
			TXT(ScreenId::MAIN, MainTextId::R_TAKEN_TIMESPAN)->hide();
		}
	}
}

void GUI::draw() {
	wake();
	log_i("Drawing screen %d", size_t(_currentScreen));
	for (auto& panel : _screens[size_t(_currentScreen)].panels) panel->draw(UPDATE_MODE_NONE);
	for (auto& text : _screens[size_t(_currentScreen)].texts) text->draw(UPDATE_MODE_NONE);
	for (auto& button : _screens[size_t(_currentScreen)].buttons) button->draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(UPDATE_MODE_GC16);
	sleep();
}

}  // namespace gui