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
		TXT_HEADER,
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

	enum ButtonIdx { /*BTN_BOOK, BTN_FREE,*/ BTN_SIZE };

	void update(std::shared_ptr<cal::CalendarStatus> status, bool draw = true);

	void show(bool show = true) override;

	void draw(m5epd_update_mode_t mode) override;

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	std::function<void()> startLoadingAnim = nullptr;
	std::function<void()> stopLoadingAnim = nullptr;
	std::function<void()> book = nullptr;

  private:
	std::shared_ptr<cal::CalendarStatus> _status = nullptr;

	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;

	Animation _batteryAnim = Animation("/images/battery", 6, 810, 18);
	uint8_t _curBatteryImage = 0;
};
}  // namespace gui

#endif