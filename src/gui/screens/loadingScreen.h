#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <array>
#include <memory>

#include "../elements/animation.h"
#include "../elements/button.h"
#include "../elements/panel.h"
#include "../elements/text.h"
#include "calendar/api.h"
#include "screen.h"

namespace gui {
class LoadingScreen : public Screen {
  public:
	LoadingScreen();
	~LoadingScreen(){};

	enum PanelIdx { PNL_MAIN, PNL_SIZE };
	enum TextIdx { TXT_LOADING_1, TXT_LOADING_2, TXT_SIZE };
	enum ButtonIdx { BTN_SIZE };

	void show(bool show = true) override;

	void draw(m5epd_update_mode_t mode) override;

	void handleTouch(int16_t x = -1, int16_t y = -1) override;

	/** Set the main loading screen text. Use a newline to utilize the second line. */
	void setText(String text);

  private:
	std::array<std::unique_ptr<Panel>, PNL_SIZE> _panels;
	std::array<std::unique_ptr<Text>, TXT_SIZE> _texts;
	std::array<std::unique_ptr<Button>, BTN_SIZE> _buttons;
};
}  // namespace gui

#endif