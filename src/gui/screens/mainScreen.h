#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include <array>
#include <memory>

#include "calendar/api.h"
#include "gui/elements/animation.h"
#include "gui/elements/button.h"
#include "gui/elements/panel.h"
#include "gui/elements/text.h"
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
		TXT_BATTERY_LEVEL,
		TXT_R_FREE_HEADER,
		TXT_R_TAKEN_HEADER,
		TXT_R_TAKEN_ORGANIZER,
		TXT_R_TAKEN_SUMMARY,
		TXT_R_TAKEN_TIMESPAN,
		TXT_ERROR,
		TXT_BATTERY_WARNING,
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

	const std::array<Pos, 6> BTN_GRID_POSITIONS{Pos{80, 306}, Pos{232, 306}, Pos{384, 306},
	                                            Pos{80, 396}, Pos{232, 396}, Pos{384, 396}};

	void setStatus(std::shared_ptr<cal::CalendarStatus> status);
	void setError(const String& error);

	void draw(m5epd_update_mode_t mode) override;

	/**
	 * This draw mode is called when we are already on the main screen.
	 * This means that we don't necessarily need to draw everything again.
	 * We do still draw everyting if state has sufficiently changed.
	 *
	 * This is a separate function to not pollute the Screen interface.
	 *
	 * We need to be diligent when calling this function.
	 * We need to make sure that the current screen surely is already this screen and there is no
	 * loading icons or anything else that needs to be drawn over.
	 */
	void reducedDraw(m5epd_update_mode_t mode);

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	// Callbacks the GUI class can register to (they fire on button presses)
	std::function<void(int minutes)> onBook = nullptr;
	std::function<void()> onBookUntilNext = nullptr;
	std::function<void()> onFree = nullptr;
	std::function<void()> onExtend = nullptr;
	std::function<void()> onGoSettings = nullptr;

  private:
	void _updateLeftSide();
	void _updateRightSide();

	/**
	 * Returns true if buttons have changed (hidden or shown).
	 */
	bool _updateButtons();

	void _drawImpl(m5epd_update_mode_t mode, bool allowReducedDraw);

	std::shared_ptr<cal::CalendarStatus> _status = nullptr;
	String _error = "";

	bool _statusChanged = true;
	bool _errorChanged = true;

	float _batteryLevel = -1;

	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;

	std::array<Animation, 2> _batteryAnim{Animation("/images/battery", 6, Pos{812, 18}),
	                                      Animation("/images/batteryDarker", 6, Pos{812, 18})};

	Image _batteryWarningIcon{"/images/battery1.png", Pos{652 + 32 + 12 + 4, 64 + 8 + 4}, true};

	BatteryStyle _batteryStyle = BATTERY_LIGHT;
	uint8_t _batteryImage = 0;
};
}  // namespace gui

#endif