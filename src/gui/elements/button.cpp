#include "button.h"

#include "M5EPD.h"
#include "utils.h"

namespace gui {

Button::Button(Pos pos, Size size, const String& text, uint8_t fontSize, uint8_t textColor,
               uint8_t bgColor, std::function<void()> callback)
    : Element(pos, size),
      _text{utils::make_unique<Text>(pos, size, text, fontSize, textColor, bgColor, true, true)},
      _callback{callback} {
	_text->show(true);
}

Button::Button(Pos pos, Size size, const String& imagePath, std::function<void()> callback)
    : Element(pos, size), _image{utils::make_unique<Image>(imagePath, pos)}, _callback{callback} {}

void Button::draw(m5epd_update_mode_t mode) {
	if (_hidden) {
		return;
	}

	if (_text) {
		_text->draw(mode);
	} else if (_image) {
		_image->draw(mode);
	}
}

void Button::handleTouch(int16_t x, int16_t y) {
	if (_hidden || _disabled) {
		return;
	}

	// log_i("Handling touch at (%d, %d)", x, y);

	// This means touch up
	if (x == -1 && y == -1 && _pressed) {
		if (_callback) {
			_callback();
		}
	}

	if (x >= 0 && y >= 0 && x >= _pos.x && x <= _pos.x + _size.w && y >= _pos.y
	    && y <= _pos.y + _size.h) {
		_pressed = true;
	} else {
		_pressed = false;
	}
}

}  // namespace gui