#ifndef CONFIRM_FREE_SCREEN_H
#define CONFIRM_FREE_SCREEN_H

#include <array>
#include <memory>

#include "calendar/api.h"
#include "configServer.h"
#include "gui/elements/animation.h"
#include "gui/elements/button.h"
#include "gui/elements/panel.h"
#include "gui/elements/text.h"
#include "screen.h"

namespace gui {
class ConfirmFreeScreen : public Screen {
  public:
	ConfirmFreeScreen();
	~ConfirmFreeScreen(){};

	enum PanelIdx { PNL_MAIN, PNL_SIZE };
	enum TextIdx { TXT_TITLE, TXT_ORGANIZER, TXT_SUMMARY, TXT_TIMESPAN, TXT_SIZE };
	enum ButtonIdx { BTN_CANCEL, BTN_CONFIRM, BTN_SIZE };

	void setEvent(std::shared_ptr<cal::Event> event);

	void draw(m5epd_update_mode_t mode) override;

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	// Callbacks the GUI class can register to (they fire on button presses)
	std::function<void()> onConfirm = nullptr;
	std::function<void()> onCancel = nullptr;

  private:
	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;

	std::unique_ptr<config::ConfigStore> _configStore = nullptr;
	std::unique_ptr<config::ConfigServer> _configServer = nullptr;
};
}  // namespace gui

#endif