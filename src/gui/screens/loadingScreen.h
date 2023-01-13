#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <array>
#include <memory>

#include "calendar/api.h"
#include "gui/elements/animation.h"
#include "gui/elements/button.h"
#include "gui/elements/panel.h"
#include "gui/elements/text.h"
#include "screen.h"

namespace gui {
class LoadingScreen : public Screen {
  public:
	LoadingScreen();
	~LoadingScreen(){};

	enum PanelIdx { PNL_MAIN, PNL_SIZE };
	enum TextIdx { TXT_LOADING_1, TXT_LOADING_2, TXT_SIZE };
	enum ButtonIdx { BTN_SIZE };

	void draw(m5epd_update_mode_t mode) override;

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	/** Set the main loading screen text. It is drawn instantly.
	 * Use a newline to utilize the second line. */
	void setText(String text);

  private:
	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;
};
}  // namespace gui

#endif