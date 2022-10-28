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

	void setColor(uint8_t color) { _color = color; }

	void draw(m5epd_update_mode_t mode) override final;

  private:
	M5EPD_Canvas _canvas;
	uint8_t _color;
};
}  // namespace gui
#endif