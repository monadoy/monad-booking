#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

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
class SettingsScreen : public Screen {
  public:
	SettingsScreen();
	~SettingsScreen(){};

	enum PanelIdx { PNL_MAIN, PNL_SIZE };
	enum TextIdx { TXT_TITLE, TXT_MAIN, TXT_SIZE };
	enum ButtonIdx { BTN_SETTINGS, BTN_UPDATE, BTN_SETUP, BTN_SIZE };

	void draw(m5epd_update_mode_t mode) override;

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	// Callbacks the GUI class can register to (they fire on button presses)
	std::function<void()> onStartUpdate = nullptr;
	std::function<void()> onGoSetup = nullptr;
	std::function<void()> onBack = nullptr;

  private:
	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;
};
}  // namespace gui

#endif