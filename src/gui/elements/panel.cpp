#include "panel.h"

#include "M5EPD.h"

namespace gui {

Panel::Panel(Pos pos, Size size, uint8_t color) : Element(pos, size), _color{color} {}

void Panel::drawToCanvas(M5EPD_Canvas& canvas) {
	canvas.fillRect(_pos.x, _pos.y, _size.w, _size.h, _color);
}

}  // namespace gui