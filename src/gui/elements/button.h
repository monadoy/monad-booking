#ifndef BUTTON_H
#define BUTTON_H

#include "element.h"
#include "image.h"
#include "text.h"

namespace gui {
class Button : public Element {
  public:
	Button(Pos pos, Size size, const String& text, uint8_t fontSize = 24, uint8_t textColor = 15,
	       uint8_t bgColor = 0, std::function<void()> callback = nullptr);
	Button(Pos pos, Size size, const String& imagePath, std::function<void()> callback = nullptr);
	~Button(){};

	void handleTouch(int16_t x, int16_t y);

	void disable(bool disabled = true) { _disabled = disabled; }
	void enable() { disable(false); }

	void drawToCanvas(M5EPD_Canvas& canvas) override;

	void registerCallback(std::function<void()> callback) { _callback = callback; }

	void setText(const String& text) {
		assert(_text);
		_text->setText(text);
	}
	void setImagePath(const String& imagePath) {
		assert(_image);
		_image->setPath(imagePath);
	}
	void setReverseColor(bool reverseColor) {
		assert(_image);
		_image->setReverseColor(reverseColor);
	}

	void setPos(Pos pos) {
		Element::setPos(pos);
		if (_image)
			_image->pos = pos;
		if (_text)
			_text->setPos(pos);
	}

  private:
	std::unique_ptr<Text> _text = nullptr;
	std::unique_ptr<Image> _image = nullptr;
	bool _disabled = false;
	std::function<void()> _callback;

	bool _pressed = false;
};
}  // namespace gui
#endif