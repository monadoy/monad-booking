#include "animation.h"

namespace gui {

Animation::Animation(String basePath, int16_t frames, Pos pos, bool reverseColor)
    : _basePath(basePath), _frames(frames) {
	for (int16_t i = 0; i < _frames; i++) {
		String path = _basePath + String(i + 1) + ".png";
		_images.emplace_back(path, pos, reverseColor);
	}
}

void Animation::drawFrame(int16_t frame, m5epd_update_mode_t updateMode) {
	if (frame >= 1 && frame <= _frames) {
		_images[frame - 1].draw(updateMode);
	} else {
		assert(false);
	}
}
void Animation::drawFrameToCanvas(int16_t frame, M5EPD_Canvas& canvas) {
	if (frame >= 1 && frame <= _frames) {
		_images[frame - 1].drawToCanvas(canvas);
	} else {
		assert(false);
	}
}

void Animation::advanceFrame() {
	_currentFrame += _animDir;
	if (_currentFrame >= _frames) {
		_currentFrame = _frames - 2;
		_animDir = -1;
	} else if (_currentFrame < 0) {
		_currentFrame = 1;
		_animDir = 1;
	}
}

void Animation::drawNext(m5epd_update_mode_t updateMode) {
	advanceFrame();
	_images[_currentFrame].draw(updateMode);
}

void Animation::drawNextToCanvas(M5EPD_Canvas& canvas) {
	advanceFrame();
	_images[_currentFrame].drawToCanvas(canvas);
}

void Animation::reset() {
	_currentFrame = 0;
	_animDir = 1;
}

}  // namespace gui