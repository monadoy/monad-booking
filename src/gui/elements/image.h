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

	Pos pos;

	void setPath(String path) { _path = path; }
	void setReverseColor(bool reverseColor) { _reverseColor = reverseColor; }

  private:
	String _path;
	bool _reverseColor = false;
};
}  // namespace gui

#endif