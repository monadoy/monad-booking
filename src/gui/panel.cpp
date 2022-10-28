#include "panel.h"

#include "M5EPD.h"

namespace gui {

Panel::Panel(Pos pos, Size size, uint8_t color)
    : Element(pos, size), _color{color}, _canvas{&M5.EPD} {
	_canvas.createCanvas(_size.w, _size.h);
}

void Panel::draw(m5epd_update_mode_t mode) {
	if (_hidden) {
		return;
	}
	log_i("Drawing panel");

	_canvas.fillCanvas(_color);
	_canvas.pushCanvas(_pos.x, _pos.y, mode);
}

}  // namespace gui