#include "animManager.h"

#include <LittleFS.h>
#include <M5EPD.h>

#define ANIM_MANAGER_BUFFER_SIZE 376 * 248 / 4

namespace anim {
File myFile;
PNG png;

void* openFunc(const char* filename, int32_t* size) {
	// Serial.printf("Attempting to open %s\n", filename);
	myFile = LittleFS.open(filename);
	*size = myFile.size();
	return &myFile;
}

void closeFunc(void* handle) {
	if (myFile)
		myFile.close();
}

int32_t readFunc(PNGFILE* handle, uint8_t* buffer, int32_t length) {
	if (!myFile)
		return 0;
	return myFile.read(buffer, length);
}

int32_t seekFunc(PNGFILE* handle, int32_t pos) {
	if (!myFile)
		return 0;
	return myFile.seek(pos);
}

void PNGDraw(PNGDRAW* pDraw) {
	// the coordinates are hard coded now
	M5.EPD.WritePartGram4bpp(292, 107 + pDraw->y, pDraw->iWidth, 1, pDraw->pPixels);
}

Animation::Animation() {
	uint8_t* animBuffer = new uint8_t[ANIM_MANAGER_BUFFER_SIZE];
	png.setBuffer(animBuffer);
	resetAnimation();
}

void Animation::showNextFrame() {
	_currentFrame += _direction;
	if (_currentFrame == NUM_OF_FRAMES || _currentFrame == 1)
		_reverseDirection();
	_drawFrame();
}

void Animation::resetAnimation() {
	_currentFrame = 1;
	_direction = 1;
}

void Animation::_drawFrame() {
	log_i("%u, %u", ESP.getFreeHeap(), ESP.getFreePsram());
	int beginTime = millis();
	M5.EPD.SetColorReverse(true);
	String pngName = "/images/frame" + String(15 - _currentFrame) + ".png";
	int rc = png.open(pngName.c_str(), openFunc, closeFunc, readFunc, seekFunc, PNGDraw);
	if (rc == PNG_SUCCESS) {
		png.decode(NULL, 0);
		M5.EPD.UpdateArea(292, 107, 376, 248, UPDATE_MODE_DU4);
	}
	M5.EPD.SetColorReverse(false);
	Serial.print("Frame drawing took ");
	Serial.print(millis() - beginTime);
	Serial.println(" milliseconds.");
	log_i("%u, %u", ESP.getFreeHeap(), ESP.getFreePsram());
	png.close();
	log_i("%u, %u", ESP.getFreeHeap(), ESP.getFreePsram());
}

void Animation::_reverseDirection() { _direction = (-1) * _direction; }

}  // namespace anim