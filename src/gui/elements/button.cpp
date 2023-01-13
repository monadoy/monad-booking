#include "button.h"

#include "M5EPD.h"
#include "utils.h"

namespace gui {

Button::Button(Pos pos, Size size, const String& text, uint8_t fontSize, uint8_t textColor,
               uint8_t bgColor, std::function<void()> callback)
    : Element(pos, size),
      _text{utils::make_unique<Text>(pos, size, text, fontSize, textColor, bgColor, true,
                                     Align::CENTER)},
      _callback{callback} {
	_text->show(true);
}

Button::Button(Pos pos, Size size, const String& imagePath, std::function<void()> callback)
    : Element(pos, size), _image{utils::make_unique<Image>(imagePath, pos)}, _callback{callback} {}

void Button::drawToCanvas(M5EPD_Canvas& canvas) {
	if (_hidden) {
		return;
	}

	if (_text) {
		_text->drawToCanvas(canvas);
	} else if (_image) {
		_image->drawToCanvas(canvas);
	}
}

void Button::handleTouch(int16_t x, int16_t y) {
	if (_hidden || _disabled) {
		return;
	}

	// log_i("Handling touch at (%d, %d)", x, y);

	if (x >= _pos.x && x <= _pos.x + _size.w && y >= _pos.y && y <= _pos.y + _size.h) {
		_callback();
	}
}

}  // namespace gui