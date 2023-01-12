#include "loadingScreen.h"

#include "globals.h"
#include "gui/displayUtils.h"

namespace gui {

LoadingScreen::LoadingScreen() {
	ADD_PNL(PNL_MAIN, Panel(Pos{0, 0}, Size{960, 540}, WH));

	ADD_TXT(TXT_LOADING_1,
	        Text(Pos{0, 402}, Size{960, 42}, "", FS_HEADER, BK, WH, false, Align::CENTER));
	ADD_TXT(TXT_LOADING_2,
	        Text(Pos{0, 444}, Size{960, 42}, "", FS_HEADER, BK, WH, false, Align::CENTER));

	ASSERT_ALL_ELEMENTS();
}

void LoadingScreen::draw(m5epd_update_mode_t mode) {
	wakeDisplay();
	for (auto& p : _panels) p->draw(UPDATE_MODE_NONE);
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	for (auto& b : _buttons) b->draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(mode);
}

void LoadingScreen::handleTouch(int16_t x, int16_t y) {
	for (auto& b : _buttons) b->handleTouch(x, y);
}

void LoadingScreen::setText(String text) {
	log_i("Setting loading screen text to: %s", text.c_str());
	int newline = text.indexOf('\n');
	if (newline == -1) {
		_texts[TXT_LOADING_1]->setText(text);
		_texts[TXT_LOADING_2]->setText("");
	} else {
		_texts[TXT_LOADING_1]->setText(text.substring(0, newline));
		_texts[TXT_LOADING_2]->setText(text.substring(newline + 1));
	}
	wakeDisplay();
	delay(30);
	// Draw only text
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateArea(0, 402, 960, 84, MY_UPDATE_MODE);
	delay(30);
}

}  // namespace gui