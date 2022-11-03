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

	enum class PanelId { LEFT, RIGHT, SIZE };
	enum class TextId {
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
	enum class ButtonId { BOOK, FREE, SIZE };

	void update(std::shared_ptr<cal::CalendarStatus> status);

	void show(bool show = true) override;

	void draw(m5epd_update_mode_t mode) override;

	std::function <void()> startLoadingAnim = nullptr;
	std::function <void()> stopLoadingAnim = nullptr;
	std::function <void()> book = nullptr;

  private:
	std::shared_ptr<cal::CalendarStatus> _status = nullptr;

	std::array<std::unique_ptr<Panel>, size_t(PanelId::SIZE)> _panels;
	std::array<std::unique_ptr<Text>, size_t(TextId::SIZE)> _texts;
	std::array<std::unique_ptr<Button>, size_t(ButtonId::SIZE)> _buttons;

	Animation _batteryAnim = Animation("/images/battery", 6, 810, 18);
	uint8_t _curBatteryImage = 0;
	Animation _logoAnim = Animation("/images/frame", 15, 292, 107);
};
}  // namespace gui

#endif