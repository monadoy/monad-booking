#ifndef TEXT_H
#define TEXT_H

#include <Arduino.h>
#include <M5EPD.h>

#include "element.h"

namespace gui {
// Only left align allows multiline text
enum class Align { LEFT, CENTER, RIGHT };

class Text : public Element {
  public:
	Text(Pos pos, Size size, const String& text, uint8_t fontSize = 24, uint8_t textColor = 15,
	     uint8_t bgColor = 0, bool bold = false, Align align = Align::LEFT,
	     Margins margins = Margins{4, 4, 4, 4});
	~Text(){};

	void setText(const String& text) { _text = text; }

	void setBGColor(uint8_t color) { _bgColor = color; }

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
	Align _align;
	M5EPD_Canvas _canvas;

	uint8_t _bgColor;

	Margins _margins;
};
}  // namespace gui
#endif