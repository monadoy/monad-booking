#ifndef GUI_H
#define GUI_H

#include <Arduino.h>

#include "calendar/model.h"
#include "elements/animation.h"
#include "elements/button.h"
#include "elements/panel.h"
#include "elements/text.h"
#include "screens/confirmFreeScreen.h"
#include "screens/loadingScreen.h"
#include "screens/mainScreen.h"
#include "screens/screen.h"
#include "screens/settingsScreen.h"
#include "screens/setupScreen.h"
#include "screens/shutdownScreen.h"

class GUITask;

namespace gui {

/**
 * Don't call methods of this class multithreaded,
 * only from GUI task or from within this class.
 */
class GUI {
  public:
	GUI(GUITask* guiTask);

	void initMain(cal::Model* model);

	enum ScreenIdx {
		SCR_LOADING,
		SCR_MAIN,
		SCR_SETTINGS,
		SCR_SETUP,
		SCR_CONFIRM_FREE,
		SCR_SHUTDOWN,
		SCR_SIZE
	};

	void switchToScreen(ScreenIdx screenId);

	void handleTouch(int16_t x = -1, int16_t y = -1);

	void showCalendarStatus(std::shared_ptr<cal::CalendarStatus> status);
	void showError(const String& error);

	void wake();
	void sleep();

	void startSetup(bool useAP);

	void startLoading();
	void stopLoading();
	void showLoadingAnimNextFrame();

	void setLoadingScreenText(String text);

	void showShutdownScreen(String message, bool isError);

  private:
	// Non owning pointers
	cal::Model* _model;
	GUITask* _guiTask;

	std::unique_ptr<LoadingScreen> _loadingScreen = nullptr;
	std::unique_ptr<MainScreen> _mainScreen = nullptr;
	std::unique_ptr<SetupScreen> _setupScreen = nullptr;
	std::unique_ptr<ConfirmFreeScreen> _confirmFreeScreen = nullptr;
	std::unique_ptr<ShutdownScreen> _shutdownScreen = nullptr;
	std::unique_ptr<SettingsScreen> _settingsScreen = nullptr;

	// Non owning pointers
	std::array<Screen*, SCR_SIZE> _screens{};

	ScreenIdx _currentScreen = SCR_LOADING;

	bool _touching = false;

	// Loading animation is overlayed on top of the current screen when needed.
	// When animating, it only updates the area it occupies and uses a faster
	// update mode to avoid lag.
	Animation _loadingAnim = Animation("/images/frame", 15, Pos{292, 107});
	bool _loading = false;

	std::shared_ptr<cal::CalendarStatus> _status = nullptr;
};
}  // namespace gui

#endif  // GUI_H