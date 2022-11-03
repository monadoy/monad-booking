#include "button.h"

#include "M5EPD.h"

namespace gui {

Button::Button(Pos pos, Size size, const String& text, uint8_t fontSize, uint8_t textColor,
               uint8_t bgColor, std::function<void()> callback)
    : Text(pos, size, text, fontSize, textColor, bgColor, true, true), _callback{callback} {}

void Button::handleTouch(int16_t x, int16_t y) {
	if (_hidden) {
		return;
	}

	log_i("Handling touch at (%d, %d)", x, y);

	if (x >= 0 && y >= 0 && x >= _pos.x && x <= _pos.x + _size.w && y >= _pos.y
	    && y <= _pos.y + _size.h) {
		log_i("Button pressed");
		if (_callback) {
			_callback();
		}
	}
}

}  // namespace gui