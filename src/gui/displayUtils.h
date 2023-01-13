#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <M5EPD.h>

#include "elements/element.h"

namespace gui {

String timeSpanStr(time_t startTime, time_t endTime);

/**
 * Put the display to sleep.
 * This wraps M5.EPD.Sleep() in a boolean check to prevent unnecessary calls.
 * DO NOT CALL M5.EPD.Sleep() DIRECTLY, it causes a de-sync!
 */
void sleepDisplay();
/**
 * Wake the display. Needs to be called before drawing.
 * This wraps M5.EPD.Active() in a boolean check to prevent unnecessary calls.
 * DO NOT CALL M5.EPD.Active() DIRECTLY, it causes a de-sync!
 */
void wakeDisplay();

void readPartFromCanvas(Pos pos, Size size, M5EPD_Canvas& canvas, uint16_t canvasWidth,
                        uint8_t* partBuffer);

/**
 * Get a canvas that can be used as a screen buffer.
 * Used by all screens, so it should be fully overwritten when drawing.
 * This function is used to avoid allocating the buffer multiple times.
 */
M5EPD_Canvas& getScreenBuffer();

};  // namespace gui

#endif