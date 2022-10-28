#include "text.h"

#include "M5EPD.h"

namespace gui {

Text::Text(Pos pos, Size size, const String& text, uint8_t fontSize, uint8_t textColor,
           uint8_t bgColor, bool bold, bool centered)
    : Element(pos, size),
      _text{text},
      _fontSize{fontSize},
      _textColor{textColor},
      _bgColor{bgColor},
      _bold{bold},
      _centered{centered},
      _canvas{&M5.EPD} {
	log_i("Creating text element %s", _text.c_str());
	_canvas.createCanvas(_size.w, _size.h);
}

void Text::draw(m5epd_update_mode_t mode) {
	if (_hidden) {
		return;
	}
	log_i("Drawing text: %s", _text.c_str());

	if (_bold) {
		_canvas.setTextFont(1);
	} else {
		_canvas.setTextFont(2);
	}

	// These seem to be globals, need to always set before draw
	_canvas.setTextSize(_fontSize);

	if (_centered) {
		_canvas.setTextDatum(CC_DATUM);
	} else {
		_canvas.setTextDatum(TL_DATUM);
	}
	_canvas.setTextColor(_textColor);

	_canvas.fillCanvas(_bgColor);
	_canvas.setTextArea(_margins.left, _margins.top, _size.w - _margins.right,
	                    _size.h - _margins.bottom);
	if (_centered) {
		_canvas.drawString(_text, _size.w / 2, _size.h / 2 + 3);
	} else {
		_canvas.print(_text);
	}
	_canvas.pushCanvas(_pos.x, _pos.y, mode);
}

}  // namespace gui