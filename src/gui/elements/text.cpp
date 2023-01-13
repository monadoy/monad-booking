#include "text.h"

#include "M5EPD.h"

namespace gui {

Text::Text(Pos pos, Size size, const String& text, uint8_t fontSize, uint8_t textColor,
           uint8_t bgColor, bool bold, Align align, Margins margins)
    : Element(pos, size),
      _text{text},
      _fontSize{fontSize},
      _textColor{textColor},
      _bgColor{bgColor},
      _bold{bold},
      _align{align},
      _margins{margins},
      _canvas{&M5.EPD} {
	// log_i("Creating text element %s", _text.c_str());
	_canvas.createCanvas(_size.w, _size.h);
}

void Text::drawToCanvas(M5EPD_Canvas& canvas) {
	if (_hidden) {
		return;
	}

	// log_i("Drawing text: %s", _text.c_str());

	if (_changed) {
		// These seem to be globals, need to always set before draw
		if (_bold) {
			_canvas.setTextFont(1);
		} else {
			_canvas.setTextFont(2);
		}

		_canvas.setTextSize(_fontSize);

		if (_align == Align::LEFT) {
			_canvas.setTextDatum(TL_DATUM);
		} else if (_align == Align::RIGHT) {
			_canvas.setTextDatum(TR_DATUM);
		} else if (_align == Align::CENTER) {
			_canvas.setTextDatum(CC_DATUM);
		}

		_canvas.setTextColor(_textColor);

		_canvas.fillCanvas(_bgColor);

		_canvas.setTextArea(_margins.left, _margins.top, _size.w - _margins.right,
		                    _size.h - _margins.bottom);

		if (_align == Align::CENTER) {
			_canvas.drawString(_text, _size.w / 2, _size.h / 2 + 3);
		} else if (_align == Align::RIGHT) {
			_canvas.drawString(_text, _size.w - _margins.right, _margins.top);
		} else {
			_canvas.print(_text);
		}
	}

	_canvas.pushToCanvas(_pos.x, _pos.y, &canvas);

	_changed = false;
}

}  // namespace gui