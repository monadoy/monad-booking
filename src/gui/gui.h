#ifndef GUI_H
#define GUI_H

#include <Arduino.h>

#include "calendar/model.h"
#include "elements/animation.h"
#include "elements/button.h"
#include "elements/panel.h"
#include "elements/text.h"
#include "screens/loadingScreen.h"
#include "screens/mainScreen.h"
#include "screens/screen.h"

class GUITask;

namespace gui {

class GUI {
  public:
	GUI(GUITask* guiTask);

	void initMain(cal::Model* model);

	enum ScreenIdx { SCR_LOADING, SCR_MAIN, SCR_CONFIRM_FREE, SCR_SIZE };

	void switchToScreen(ScreenIdx screenId);

	void handleTouch(int16_t x = -1, int16_t y = -1);

	void setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status);

	void wake();
	void sleep();

	void startLoadingAnim();
	void stopLoadingAnim();
	void showLoadingAnimNextFrame();

	void setLoadingScreenText(String text);

	cal::Model* _model;
	GUITask* _guiTask;

	std::unique_ptr<LoadingScreen> _loadingScreen = nullptr;
	std::unique_ptr<MainScreen> _mainScreen = nullptr;

	std::array<Screen*, SCR_SIZE> _screens{};

	ScreenIdx _currentScreen = SCR_LOADING;

	// Loading animation is overlayed on top of the current screen when needed.
	// When animating, it only updates the area it occupies and uses a faster
	// update mode to avoid lag.
	Animation _loadingAnim = Animation("/images/frame", 15, 292, 107);
	bool _loadingAnimActive = false;

	M5EPD_Canvas _canvas;
};
}  // namespace gui

#endif  // GUI_H