#include "gui.h"

#include "LittleFS.h"
#include "globals.h"
#include "guiTask.h"

namespace gui {

const char* FONT_BOLD = "/interbold.ttf";
const char* FONT_REGULAR = "/interregular.ttf";

GUI::GUI(GUITask* guiTask) : _guiTask(guiTask) {
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
	_loadingScreen = utils::make_unique<LoadingScreen>();
	_screens[SCR_LOADING] = _loadingScreen.get();

	switchToScreen(SCR_LOADING);
}

void GUI::initMain(cal::Model* model) {
	_model = model;

	// Initialize the rest of the screens
	_mainScreen = utils::make_unique<MainScreen>();
	_screens[SCR_MAIN] = _mainScreen.get();
}

void GUI::handleTouch(int16_t x, int16_t y) { _screens[_currentScreen]->handleTouch(x, y); }

void GUI::wake() { M5.EPD.Active(); }

void GUI::sleep() {
	if (_currentScreen != SCR_MAIN) {
		switchToScreen(SCR_MAIN);
	}

	M5.EPD.Sleep();
}

void GUI::switchToScreen(ScreenIdx screenId) {
	assert(_screens[screenId] != nullptr);

	log_i("Switching to screen %d", _currentScreen);

	_screens[_currentScreen]->hide();

	_screens[screenId]->show();
	M5.EPD.Active();
	_screens[screenId]->draw(UPDATE_MODE_GC16);

	_currentScreen = screenId;
}

void GUI::setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status) {
	log_i("Setting calendar status");
	stopLoadingAnim();

	if (_currentScreen != SCR_MAIN) {  // TODO: maybe don't switch screen from options
		_mainScreen->update(status, false);
		switchToScreen(SCR_MAIN);
	} else {
		_mainScreen->update(status, true);
	}
}

void GUI::startLoadingAnim() {
	_loadingAnimActive = true;
	_guiTask->enqueueLoadingAnimNextFrame();
}

void GUI::stopLoadingAnim() {
	_loadingAnimActive = false;
	_loadingAnim.reset();
}

void GUI::showLoadingAnimNextFrame() {
	if (!_loadingAnimActive)
		return;

	_loadingAnim.drawNext(UPDATE_MODE_DU4);
	_guiTask->enqueueLoadingAnimNextFrame();
}

void GUI::setLoadingScreenText(String text) { _loadingScreen->setText(text); }

}  // namespace gui
