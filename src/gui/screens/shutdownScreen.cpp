#include "shutdownScreen.h"

#include "globals.h"

namespace gui {

ShutdownScreen::ShutdownScreen() {
	ADD_PNL(PNL_MAIN, Panel(Pos{0, 0}, Size{960, 540}, WH));

	ADD_TXT(TXT_1, Text(Pos{0, 402}, Size{960, 42}, "", FS_HEADER, BK, WH, false, true));
	ADD_TXT(TXT_2, Text(Pos{0, 444}, Size{960, 42}, "", FS_HEADER, BK, WH, false, true));

	ASSERT_ALL_ELEMENTS();
}

void ShutdownScreen::show(bool doShow) {
	Screen::show(doShow);

	for (auto& p : _panels) p->show(doShow);
	for (auto& t : _texts) t->show(doShow);
	for (auto& b : _buttons) b->show(doShow);
}

void ShutdownScreen::draw(m5epd_update_mode_t mode) {
	M5.EPD.Active();
	for (auto& p : _panels) p->draw(UPDATE_MODE_NONE);
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	for (auto& b : _buttons) b->draw(UPDATE_MODE_NONE);
	_logo.draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(mode);
}

void ShutdownScreen::setText(String text) {
	log_i("Setting loading screen text to: %s", text.c_str());
	int newline = text.indexOf('\n');
	if (newline == -1) {
		_texts[TXT_1]->setText(text);
		_texts[TXT_2]->setText("");
	} else {
		_texts[TXT_1]->setText(text.substring(0, newline));
		_texts[TXT_2]->setText(text.substring(newline + 1));
	}
}

}  // namespace gui