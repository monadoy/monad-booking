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

	// Initialize screens (only loading, setup and shutdown screen for now)
	_loadingScreen = utils::make_unique<LoadingScreen>();
	_screens[SCR_LOADING] = _loadingScreen.get();

	_setupScreen = utils::make_unique<SetupScreen>();
	_screens[SCR_SETUP] = _setupScreen.get();

	_shutdownScreen = utils::make_unique<ShutdownScreen>();
	_screens[SCR_SHUTDOWN] = _shutdownScreen.get();

	switchToScreen(SCR_LOADING);
}

void GUI::initMain(cal::Model* model) {
	_model = model;

	// Initialize the rest of the screens

	// CONFIRM FREE SCREEN
	_confirmFreeScreen = utils::make_unique<ConfirmFreeScreen>();
	_screens[SCR_CONFIRM_FREE] = _confirmFreeScreen.get();
	_confirmFreeScreen->onConfirm = [this]() {
		if (_loading)
			return;
		_model->endCurrentEvent();
		startLoading();
	};
	_confirmFreeScreen->onCancel = [this]() { switchToScreen(SCR_MAIN); };

	// SETTINGS SCREEN
	_settingsScreen = utils::make_unique<SettingsScreen>();
	_screens[SCR_SETTINGS] = _settingsScreen.get();
	_settingsScreen->onBack = [this]() { switchToScreen(SCR_MAIN); };
	_settingsScreen->onGoSetup = [this]() { startSetup(false); };
	_settingsScreen->onStartUpdate = [this]() {
		preferences.putBool(MANUAL_UPDATE_KEY, true);
		utils::forceRestart();
	};

	// MAIN SCREEN
	_mainScreen = utils::make_unique<MainScreen>();
	_screens[SCR_MAIN] = _mainScreen.get();
	_mainScreen->onBook = [this](int minutes) {
		if (_loading)
			return;

		auto res = _model->calculateReserveParams(minutes * SECS_PER_MIN);
		if (res.isErr()) {
			showError(res.err()->message);
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
			showError(res.err()->message);
			log_i("Error calculating reserve params: %s", res.err()->message.c_str());
			return;
		}
		_model->reserveEvent(*res.ok());
		startLoading();
	};
	_mainScreen->onFree = [this]() {
		if (!_status || !_status->currentEvent)
			return;
		_confirmFreeScreen->setEvent(_status->currentEvent);
		switchToScreen(SCR_CONFIRM_FREE);
	};
	_mainScreen->onExtend = [this]() {
		if (_loading)
			return;
		_model->extendCurrentEvent(15 * 60);
		startLoading();
	};
	_mainScreen->onGoSettings = [this]() { switchToScreen(SCR_SETTINGS); };
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

	_currentScreen = screenId;
	_screens[screenId]->draw(UPDATE_MODE_GC16);
}

void GUI::showCalendarStatus(std::shared_ptr<cal::CalendarStatus> status) {
	log_i("Setting calendar status");
	stopLoading();

	if (status)
		_status = status;

	_mainScreen->setStatus(status);

	if (_currentScreen == SCR_MAIN) {
		_mainScreen->draw(UPDATE_MODE_GC16);
	} else if (_currentScreen == SCR_LOADING || _currentScreen == SCR_CONFIRM_FREE) {
		switchToScreen(SCR_MAIN);
	}
}

void GUI::showError(const String& error) {
	log_i("Showing error");
	stopLoading();

	_mainScreen->setError(error);

	if (_currentScreen == SCR_MAIN) {
		_mainScreen->draw(UPDATE_MODE_GC16);
	} else if (_currentScreen == SCR_LOADING || _currentScreen == SCR_CONFIRM_FREE) {
		switchToScreen(SCR_MAIN);
	}
}

void GUI::startSetup(bool useAP) {
	_setupScreen->startSetup(useAP);
	switchToScreen(SCR_SETUP);
}

void GUI::showShutdownScreen(String message) {
	// We don't want to override our previous shutdown screen, it probably contains an error message
	if (_currentScreen == SCR_SHUTDOWN)
		return;

	if (_loading) {
		_loading = false;
	}

	_shutdownScreen->setText(message);
	switchToScreen(SCR_SHUTDOWN);
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
