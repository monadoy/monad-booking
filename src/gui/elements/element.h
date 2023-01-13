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

	virtual void drawToCanvas(M5EPD_Canvas& canvas) = 0;

	void show(bool show = true) {
		_hidden = !show;
		_changed = true;
	}
	void hide() {
		_hidden = true;
		_changed = true;
	}
	bool isHidden() { return _hidden; }

	void setPos(Pos pos) {
		_pos = pos;
		_changed = true;
	}

  protected:
	Pos _pos;
	Size _size;
	bool _hidden;

	bool _changed = true;
};
}  // namespace gui

#endif
