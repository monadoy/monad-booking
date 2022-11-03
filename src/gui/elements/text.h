#ifndef TEXT_H
#define TEXT_H

#include <Arduino.h>
#include <M5EPD.h>

#include "element.h"

namespace gui {
class Text : public Element {
  public:
	Text(Pos pos, Size size, const String& text, uint8_t fontSize = 24, uint8_t textColor = 15,
	     uint8_t bgColor = 0, bool bold = false, bool centered = false);
	~Text(){};

	void setText(const String& text) { _text = text; }

	void setColors(uint8_t textColor, uint8_t bgColor) {
		_textColor = textColor;
		_bgColor = bgColor;
	}

	void draw(m5epd_update_mode_t mode) override;

  private:
	String _text;
	uint16_t _fontSize;
	uint8_t _textColor;
	bool _bold;
	bool _centered;
	M5EPD_Canvas _canvas;

	uint8_t _bgColor;

	Margins _margins = {4, 4, 4, 4};
};
}  // namespace gui
#endif