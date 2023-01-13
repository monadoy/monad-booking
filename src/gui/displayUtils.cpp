
#include "displayUtils.h"

#include "calendar/api.h"
#include "globals.h"

namespace gui {

String timeSpanStr(time_t startTime, time_t endTime) {
	return safeMyTZ.dateTime(startTime, UTC_TIME, "G:i") + " - "
	       + safeMyTZ.dateTime(endTime, UTC_TIME, "G:i");
}

bool active = true;

void sleepDisplay() {
	if (!active)
		return;
	active = false;
	M5.EPD.Sleep();
}

void wakeDisplay() {
	if (active)
		return;
	active = true;
	M5.EPD.Active();
}

void readPartFromCanvas(Pos pos, Size size, M5EPD_Canvas& canvas, uint16_t canvasWidth,
                        uint8_t* partBuffer) {
	uint8_t* frameBuffer = (uint8_t*)canvas.frameBuffer();
	for (uint16_t y = 0; y < size.h; y++) {
		for (uint16_t x = 0; x < size.w / 2; x++) {
			auto x_idx = pos.x / 2 + x;
			auto y_idx = pos.y + y;
			auto idx = y_idx * canvasWidth / 2 + x_idx;

			partBuffer[y * size.w / 2 + x] = frameBuffer[idx];
		}
	}
}

M5EPD_Canvas& getScreenBuffer() {
	static M5EPD_Canvas canvas(&M5.EPD);
	// Does nothing is already created
	canvas.createCanvas(M5EPD_PANEL_W, M5EPD_PANEL_H);
	return canvas;
}

};  // namespace gui
