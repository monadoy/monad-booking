#ifndef GUI_H
#define GUI_H

#include <Arduino.h>

#include "animation.h"
#include "button.h"
#include "calendar/model.h"
#include "panel.h"
#include "text.h"

namespace gui {
struct Screen {
	std::vector<std::unique_ptr<Panel>> panels{};
	std::vector<std::unique_ptr<Text>> texts{};
	std::vector<std::unique_ptr<Button>> buttons{};
};

class GUI {
  public:
	GUI();

	void initMain(cal::Model* model);

	enum class ScreenId { LOADING, MAIN, CONFIRM_FREE, SIZE };

	enum class LoadingPanelId { SIZE };
	enum class LoadingTextId { SIZE };
	enum class LoadingButtonId { SIZE };

	enum class MainPanelId { LEFT, RIGHT, SIZE };
	enum class MainTextId {
		TOP_CLOCK,
		MID_CLOCK,
		ROOM_NAME,
		HEADER,
		L_FREE_SUBHEADER,
		L_TAKEN_ORGANIZER,
		L_TAKEN_SUMMARY,
		L_TAKEN_TIMESPAN,
		R_FREE_HEADER,
		R_TAKEN_HEADER,
		R_TAKEN_ORGANIZER,
		R_TAKEN_SUMMARY,
		R_TAKEN_TIMESPAN,
		SIZE
	};
	enum class MainButtonId { BOOK, FREE, SIZE };

	enum class ConfirmFreePanelId { SIZE };
	enum class ConfirmFreeTextId { SIZE };
	enum class ConfirmFreeButtonId { SIZE };

	void switchToScreen(ScreenId screenId);

	void setCalendarStatus(std::shared_ptr<cal::CalendarStatus> status);

	void handleTouch(int16_t x = -1, int16_t y = -1);

	void draw();

	void wake();
	void sleep();

	// TODO: panels

	// Need to be careful with these to keep enums in sync
	std::array<Screen, size_t(ScreenId::SIZE)> _screens{};

	Animation _batteryAnim = Animation("/images/battery", 6, 810, 18);
	Animation _logoAnim = Animation("/images/frame", 15, 292, 107);

	ScreenId _currentScreen = ScreenId::LOADING;

	cal::Model* _model;

	std::shared_ptr<cal::CalendarStatus> _status = nullptr;

	M5EPD_Canvas _canvas;
};
}  // namespace gui

#endif  // GUI_H