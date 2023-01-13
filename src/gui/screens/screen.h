#ifndef SCREEN_H
#define SCREEN_H

#include <M5EPD.h>

namespace gui {

#define ADD_PNL(id, p) _panels[id] = std::unique_ptr<Panel>(new p)
#define ADD_TXT(id, t) _texts[id] = std::unique_ptr<Text>(new t)
#define ADD_BTN(id, b) _buttons[id] = std::unique_ptr<Button>(new b)

// Assert that there are no null elements
#define ASSERT_ALL_ELEMENTS()          \
	for (auto& p : _panels) assert(p); \
	for (auto& t : _texts) assert(t);  \
	for (auto& b : _buttons) assert(b)

// FONT SIZES
const uint8_t FS_BOOTLOG = 16;
const uint8_t FS_NORMAL = 24;
const uint8_t FS_BUTTON = 28;
const uint8_t FS_HEADER = 32;
const uint8_t FS_TIMESPAN = 44;
const uint8_t FS_TITLE = 64;
const uint8_t BK = 15;  // Black
const uint8_t WH = 0;   // White

// Panel colors
const uint8_t L_PNL = 0;
const uint8_t L_PNL_TAKEN = 15;
const uint8_t R_PNL = 1;
const uint8_t R_PNL_TAKEN = 3;

class Screen {
  public:
	Screen() = default;
	virtual ~Screen() = default;

	virtual void draw(m5epd_update_mode_t mode) = 0;

	virtual void handleTouch(int16_t x = -1, int16_t y = -1) = 0;
};
}  // namespace gui

#endif