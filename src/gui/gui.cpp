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
	font.createRender(FS_TIMESPAN, 128);

	font.setTextFont(2);
	font.loadFont(FONT_REGULAR, LittleFS);
	font.createRender(FS_NORMAL, 64);
	font.createRender(FS_HEADER, 64);
	font.createRender(FS_BOOTLOG, 64);

	// Initialize screens (only loading screen and setup screen for now)
	_loadingScreen = utils::make_unique<LoadingScreen>();
	_screens[SCR_LOADING] = _loadingScreen.get();

	_setupScreen = utils::make_unique<SetupScreen>();
	_screens[SCR_SETUP] = _setupScreen.get();

	switchToScreen(SCR_LOADING);
}

void GUI::initMain(cal::Model* model) {
	_model = model;

	// Initialize the rest of the screens
	_confirmFreeScreen = utils::make_unique<ConfirmFreeScreen>();
	_screens[SCR_CONFIRM_FREE] = _confirmFreeScreen.get();
	_confirmFreeScreen->onConfirm = [this]() {
		if (_loading)
			return;
		_model->endCurrentEvent();
		startLoading();
	};
	_confirmFreeScreen->onCancel = [this]() { switchToScreen(SCR_MAIN); };

	_mainScreen = utils::make_unique<MainScreen>();
	_screens[SCR_MAIN] = _mainScreen.get();
	_mainScreen->onBook = [this](int minutes) {
		if (_loading)
			return;

		auto res = _model->calculateReserveParams(minutes * 60);
		if (res.isErr()) {
			// TODO: show error
			log_i("Error calculating reserve params: %s", res.err()->message.c_str());
			return;
		}

		_model->reserveEvent(*res.ok());
		startLoading();
	};
	_mainScreen->onBookUntilNext = [this]() {
		if (_loading)
			return;
		auto res = _model->calculateReserveUntilNextParams();
		if (res.isErr()) {
			// TODO: show error
			return;
		}
		_model->reserveEvent(*res.ok());
		startLoading();
	};
	_mainScreen->onFree = [this]() {
		if (!_status || !_status->currentEvent)
			return;
		_confirmFreeScreen->showEvent(_status->currentEvent);
		switchToScreen(SCR_CONFIRM_FREE);
	};
	_mainScreen->onExtend = [this]() {
		if (_loading)
			return;
		_model->extendCurrentEvent(15 * 60);
		startLoading();
	};
}

void GUI::handleTouch(int16_t x, int16_t y) {
	sleepManager.refreshTouchWake();
	_screens[_currentScreen]->handleTouch(x, y);
}

void GUI::wake() {}

void GUI::sleep() {
	log_i("Setting screen to sleep");
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
	_screens[screenId]->draw(UPDATE_MODE_GC16);

	_currentScreen = screenId;
}

void GUI::setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status) {
	log_i("Setting calendar status");
	stopLoading();

	if (status)
		_status = status;

	if (_currentScreen != SCR_MAIN) {  // TODO: maybe don't switch screen from options
		_mainScreen->update(status, false);
		switchToScreen(SCR_MAIN);
	} else {
		_mainScreen->update(status, true);
	}
}

void GUI::startSetup(bool useAP) {
	_setupScreen->startSetup(useAP);
	switchToScreen(SCR_SETUP);
}

void GUI::startLoading() {
	M5.EPD.Active();
	_loading = true;
	_guiTask->loadingAnimNextFrame();
}

void GUI::stopLoading() {
	_loading = false;
	_loadingAnim.reset();
}

void GUI::showLoadingAnimNextFrame() {
	if (!_loading)
		return;

	_loadingAnim.drawNext(UPDATE_MODE_DU4);
	_guiTask->loadingAnimNextFrame();
}

void GUI::setLoadingScreenText(String text) { _loadingScreen->setText(text); }

}  // namespace gui
