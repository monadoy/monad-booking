#include "animManager.h"
#include <M5EPD.h>
#include <LittleFS.h>

File myFile;
PNG png;

void* openFunc(const char *filename, int32_t *size) {
    Serial.printf("Attempting to open %s\n", filename);
    myFile = LittleFS.open(filename);
    *size = myFile.size();
    return &myFile;
}

void closeFunc(void *handle) {
    if(myFile)
        myFile.close();
}

int32_t readFunc(PNGFILE *handle, uint8_t *buffer, int32_t length) {
    if(!myFile)
        return 0;
    return myFile.read(buffer, length);
}

int32_t seekFunc(PNGFILE *handle, int32_t pos) {
    if(!myFile)
        return 0;
    return myFile.seek(pos);
}

void PNGDraw(PNGDRAW *pDraw)
{
    // the coordinates are hard coded now
    uint8_t usPixels[375];
    static const uint8_t *pixels = usPixels;
    M5.EPD.WritePartGram4bpp(521, 350+pDraw->y, pDraw->iWidth, 1, pixels);
}

void showLoadingAnimation() {
    int rc;
	for (int i = 1; i <= NUM_OF_FRAMES; i++) {
        Serial.println("Draw frame");
		String pngName = "/images/frame" + String(16 - i) + ".png";
        rc = png.open(pngName.c_str(), openFunc, closeFunc, readFunc, seekFunc, PNGDraw);
        M5.EPD.UpdateArea(521, 350, 375, 248, UPDATE_MODE_DU4);
	}
}