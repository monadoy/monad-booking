#ifndef IMAGE_H
#define IMAGE_H

#include <Arduino.h>
#include <M5EPD.h>

namespace gui {

/**
 * Images aren't thread-safe, call animations only from the gui thread.
 */
class Image {
  public:
	Image(String path, uint16_t x, uint16_t y, bool reverseColor = false);

	void draw(m5epd_update_mode_t updateMode);

	uint16_t x = 0;
	uint16_t y = 0;

  private:
	const String _path;
	const bool _reverseColor = false;
};
}  // namespace gui

#endif