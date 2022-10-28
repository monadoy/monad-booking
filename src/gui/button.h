#ifndef BUTTON_H
#define BUTTON_H

#include "element.h"
#include "text.h"

namespace gui {
class Button : public Text {
  public:
	Button(Pos pos, Size size, const String& text, uint8_t fontSize = 24, uint8_t textColor = 15,
	       uint8_t bgColor = 0, std::function<void()> callback = nullptr);
	~Button(){};

	void handleTouch(int16_t x, int16_t y);

	void registerCallback(std::function<void()> callback) { _callback = callback; }

  private:
	std::function<void()> _callback;
};
}  // namespace gui
#endif