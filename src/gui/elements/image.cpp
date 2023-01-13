#include "image.h"

#include "LittleFS.h"
#include "globals.h"

namespace gui {

void* openFunc(const char* filename, int32_t* size) {
	log_d("Opening file %s", filename);
	File* file = new File(LittleFS.open(filename));
	*size = file->size();
	return file;
}

void closeFunc(void* handle) {
	log_d("Closing file %p", handle);
	if (handle) {
		File* file = static_cast<File*>(handle);
		file->close();
		delete file;
	}
}

int32_t readFunc(PNGFILE* handle, uint8_t* buffer, int32_t length) {
	if (!handle)
		return 0;
	return static_cast<File*>(handle->fHandle)->read(buffer, length);
}

int32_t seekFunc(PNGFILE* handle, int32_t pos) {
	if (!handle)
		return 0;
	return static_cast<File*>(handle->fHandle)->seek(pos);
}

void PNGDraw(PNGDRAW* pDraw) {
	Image* image = static_cast<Image*>(pDraw->pUser);
	M5.EPD.WritePartGram4bpp(image->pos.x, image->pos.y + pDraw->y, pDraw->iWidth, 1,
	                         pDraw->pPixels);
}

void PNGDrawToCanvas(PNGDRAW* pDraw) {
	Image* image = static_cast<Image*>(pDraw->pUser);
	image->_canvas->pushImage(image->pos.x, image->pos.y + pDraw->y, pDraw->iWidth, 1,
	                          pDraw->pPixels);
	if (!image->_reverseColor)
		image->_canvas->ReversePartColor(image->pos.x, image->pos.y + pDraw->y, pDraw->iWidth, 1);
}

Image::Image(String path, Pos pos, bool reverseColor)
    : pos{pos}, _path(path), _reverseColor(reverseColor) {}

void Image::draw(m5epd_update_mode_t updateMode) {
	int beginTime = millis();

	// White parts of the image actually show as black on the screen
	// This is why we set M5.EPD.SetColorReverse(true) when reverseColor is false
	if (!_reverseColor)
		M5.EPD.SetColorReverse(true);

	int res1 = png.open(_path.c_str(), openFunc, closeFunc, readFunc, seekFunc, PNGDraw);
	if (res1 == PNG_SUCCESS) {
		int res2 = png.decode(this, 0);
		if (res2 == PNG_SUCCESS && updateMode != UPDATE_MODE_NONE) {
			M5.EPD.UpdateArea(pos.x, pos.y, png.getWidth(), png.getHeight(), updateMode);
		}
	}
	if (!_reverseColor)
		M5.EPD.SetColorReverse(false);
	log_d("Frame drawing took %u ms.", millis() - beginTime);
	png.close();
}

void Image::drawToCanvas(M5EPD_Canvas& canvas) {
	int beginTime = millis();

	_canvas = &canvas;
	int res1 = png.open(_path.c_str(), openFunc, closeFunc, readFunc, seekFunc, PNGDrawToCanvas);
	if (res1 == PNG_SUCCESS) {
		int res2 = png.decode(this, 0);
	}
	log_d("Frame drawing took %u ms.", millis() - beginTime);
	png.close();
}

}  // namespace gui