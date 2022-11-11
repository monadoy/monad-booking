#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include <array>
#include <memory>

#include "../elements/animation.h"
#include "../elements/button.h"
#include "../elements/panel.h"
#include "../elements/text.h"
#include "calendar/api.h"
#include "screen.h"

namespace gui {
class MainScreen : public Screen {
  public:
	MainScreen();
	~MainScreen(){};

	enum PanelIdx { PNL_LEFT, PNL_RIGHT, PNL_SIZE };
	enum TextIdx {
		TXT_MID_CLOCK,
		TXT_ROOM_NAME,
		TXT_TITLE,
		TXT_L_FREE_SUBHEADER,
		TXT_L_TAKEN_ORGANIZER,
		TXT_L_TAKEN_SUMMARY,
		TXT_L_TAKEN_TIMESPAN,
		TXT_TOP_CLOCK,
		TXT_R_FREE_HEADER,
		TXT_R_TAKEN_HEADER,
		TXT_R_TAKEN_ORGANIZER,
		TXT_R_TAKEN_SUMMARY,
		TXT_R_TAKEN_TIMESPAN,
		TXT_SIZE
	};

	enum ButtonIdx {
		BTN_SETTINGS,
		BTN_15,
		BTN_30,
		BTN_60,
		BTN_90,
		BTN_UNTIL_NEXT,
		BTN_FREE_ROOM,
		BTN_EXTEND_15,
		BTN_SIZE
	};

	enum BatteryStyle { BATTERY_LIGHT, BATTERY_DARKER };

	// How buttons are placed on the main screen.
	// First index is column, second is row.
	const std::array<Pos, 6> BTN_GRID_POSITIONS{Pos{80, 306}, Pos{232, 306}, Pos{384, 306},
	                                            Pos{80, 396}, Pos{232, 396}, Pos{384, 396}};

	void update(std::shared_ptr<cal::CalendarStatus> status, bool draw = true);

	void show(bool doShow = true) override;

	void draw(m5epd_update_mode_t mode) override;

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	// Callbacks the GUI class can register to (they fire on button presses)
	std::function<void(int minutes)> onBook = nullptr;
	std::function<void()> onBookUntilNext = nullptr;
	std::function<void()> onFree = nullptr;
	std::function<void()> onExtend = nullptr;

  private:
	std::shared_ptr<cal::CalendarStatus> _status = nullptr;

	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;

	std::array<Animation, 2> _batteryAnim{Animation("/images/battery", 6, Pos{812, 18}),
	                                      Animation("/images/batteryDarker", 6, Pos{812, 18})};
	BatteryStyle _curBatteryStyle = BATTERY_LIGHT;
	uint8_t _curBatteryImage = 0;
};
}  // namespace gui

#endif