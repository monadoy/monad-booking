#ifndef ANIMATION_H
#define ANIMATION_H

#include <Arduino.h>
#include <M5EPD.h>

#include "element.h"
#include "image.h"

namespace gui {

/**
 * Animations aren't thread-safe, call animations only from the gui thread.
 * Frame image names are 1 indexed.
 */
class Animation {
  public:
	Animation(String basePath, int16_t frames, Pos pos, bool reverseColor = false);

	/**
	 * Draw a specific frame of the animation.
	 * Won't update the internal current frame like drawNext().
	 * @param frame The frame to draw, 1 indexed.
	 */
	void drawFrame(int16_t frame, m5epd_update_mode_t updateMode);

	void drawFrameToCanvas(int16_t frame, M5EPD_Canvas& canvas);

	/**
	 * Draws the next frame of the animation.
	 * If the animation is at the end, it will loop by going backwards.
	 */
	void drawNext(m5epd_update_mode_t updateMode);
	void drawNextToCanvas(M5EPD_Canvas& canvas);

	void reset();

  private:
	void advanceFrame();

	const String _basePath;
	const int16_t _frames;
	int8_t _animDir = 1;        // 1 = forward, -1 = backward
	int16_t _currentFrame = 0;  // This one is zero indexed, bc is used to index into _images
	std::vector<Image> _images;
};
}  // namespace gui

#endif