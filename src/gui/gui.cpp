#include "gui.h"

#include "LittleFS.h"
#include "globals.h"

namespace gui {

const char* FONT_BOLD = "/interbold.ttf";
const char* FONT_REGULAR = "/interregular.ttf";

#define PNL(s, p) _screens[size_t(s)].panels[size_t(p)]
#define TXT(s, t) _screens[size_t(s)].texts[size_t(t)]
#define BTN(s, b) _screens[size_t(s)].buttons[size_t(b)]

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

	_mainScreen = utils::make_unique<MainScreen>();
	_screens[size_t(ScreenId::MAIN)] = _mainScreen.get();
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

	_screens[size_t(_currentScreen)]->hide();
	_screens[size_t(screenId)]->show();
	_currentScreen = screenId;
}

void GUI::setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status) {
	_mainScreen->update(status);

	if (_currentScreen != ScreenId::MAIN) {
		switchToScreen(ScreenId::MAIN);
	}
}

}  // namespace gui