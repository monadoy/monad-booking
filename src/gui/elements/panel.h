#ifndef PANEL_H
#define PANEL_H

#include <Arduino.h>
#include <M5EPD.h>

#include "element.h"

namespace gui {
class Panel : public Element {
  public:
	Panel(Pos pos, Size size, uint8_t color);
	~Panel(){};

	void setColor(uint8_t color) {
		if (_color == color)
			return;
		_color = color;
		_changed = true;
	}

	void drawToCanvas(M5EPD_Canvas& canvas) override;

  private:
	uint8_t _color;
};
}  // namespace gui
#endif