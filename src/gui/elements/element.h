#ifndef ELEMENT_H
#define ELEMENT_H

#include <M5EPD.h>

#include "stdint.h"

namespace gui {
struct Pos {
	uint16_t x;
	uint16_t y;
};

struct Size {
	uint16_t w;
	uint16_t h;
};

struct Margins {
	uint16_t top;
	uint16_t right;
	uint16_t bottom;
	uint16_t left;
};

class Element {
  public:
	Element(Pos pos, Size size, bool hidden = false) : _pos{pos}, _size{size}, _hidden{hidden} {};
	virtual ~Element() = default;

	virtual void draw(m5epd_update_mode_t mode) = 0;

	void show(bool show = true) { _hidden = !show; }
	void hide() { _hidden = true; }
	bool isHidden() { return _hidden; }

	void setPos(Pos pos) { _pos = pos; }

  protected:
	Pos _pos;
	Size _size;
	bool _hidden;
};
}  // namespace gui

#endif
