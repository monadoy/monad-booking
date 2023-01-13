#ifndef IMAGE_H
#define IMAGE_H

#include <Arduino.h>
#include <M5EPD.h>

#include "element.h"

namespace gui {

/**
 * Images aren't thread-safe, call animations only from the gui thread.
 */
class Image {
  public:
	Image(String path, Pos pos, bool reverseColor = false);

	void draw(m5epd_update_mode_t updateMode);
	void drawToCanvas(M5EPD_Canvas& canvas);

	Pos pos;

	void setPath(String path) { _path = path; }
	void setReverseColor(bool reverseColor) { _reverseColor = reverseColor; }

	M5EPD_Canvas* _canvas = nullptr;
	bool _reverseColor = false;

  private:
	String _path;
};
}  // namespace gui

#endif