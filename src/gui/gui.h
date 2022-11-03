#ifndef GUI_H
#define GUI_H

#include <Arduino.h>

#include "calendar/model.h"
#include "elements/animation.h"
#include "elements/button.h"
#include "elements/panel.h"
#include "elements/text.h"
#include "screens/mainScreen.h"
#include "screens/screen.h"

namespace gui {

class GUI {
  public:
	GUI();

	void initMain(cal::Model* model);

	enum class ScreenId { LOADING, MAIN, CONFIRM_FREE, SIZE };

	enum class LoadingPanelId { SIZE };
	enum class LoadingTextId { SIZE };
	enum class LoadingButtonId { SIZE };

	enum class ConfirmFreePanelId { SIZE };
	enum class ConfirmFreeTextId { SIZE };
	enum class ConfirmFreeButtonId { SIZE };

	void switchToScreen(ScreenId screenId);

	void handleTouch(int16_t x = -1, int16_t y = -1);

	void setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status);

	void wake();
	void sleep();

	std::unique_ptr<MainScreen> _mainScreen = nullptr;
	std::array<Screen*, size_t(ScreenId::SIZE)> _screens{};

	ScreenId _currentScreen = ScreenId::LOADING;

	cal::Model* _model;

	std::shared_ptr<cal::CalendarStatus> _status = nullptr;

	M5EPD_Canvas _canvas;
};
}  // namespace gui

#endif  // GUI_H