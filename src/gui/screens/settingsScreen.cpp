#include "settingsScreen.h"

#include "globals.h"
#include "gui/displayUtils.h"

namespace gui {

SettingsScreen::SettingsScreen() {
	const int txt_pad_x = 60;
	const int txt_pad_y = 120;
	const int txt_w = 960 - 2 * txt_pad_x;

	ADD_PNL(PNL_MAIN, Panel(Pos{0, 0}, Size{960, 540}, WH));

	ADD_TXT(TXT_TITLE, Text(Pos{txt_pad_x, txt_pad_y}, Size{txt_w, 78},
	                        l10n.msg(L10nMessage::SETTINGS), FS_TITLE, BK, WH, true));

	String mainText
	    = l10n.msg(L10nMessage::VERSION) + ": " + CURRENT_VERSION + " (" + UPDATE_CHANNEL + ")\n";
	if (latestVersionResult.isOk()) {
		mainText += l10n.msg(L10nMessage::LATEST_VERSION) + ": " + *latestVersionResult.ok();
	}
	ADD_TXT(TXT_MAIN,
	        Text(Pos{txt_pad_x, txt_pad_y + 78 + 20}, Size{txt_w, 540 - 78 - 20 - txt_pad_y * 2},
	             mainText, FS_NORMAL, BK, WH));

	ADD_BTN(BTN_SETTINGS,
	        Button(Pos{15, 15}, Size{64, 56}, "/images/settingsWhite.png", [this]() { onBack(); }));
	ADD_BTN(BTN_UPDATE, Button(Pos{420, 400}, Size{200, 78}, l10n.msg(L10nMessage::UPDATE),
	                           FS_BUTTON, BK, WH, [this]() { onStartUpdate(); }));
	ADD_BTN(BTN_SETUP, Button(Pos{420 + 200 + 12, 400}, Size{200, 78}, "SETUP", FS_BUTTON, WH, BK,
	                          [this]() { onGoSetup(); }));

	ASSERT_ALL_ELEMENTS();
}

void SettingsScreen::draw(m5epd_update_mode_t mode) {
	_buttons[BTN_UPDATE]->show(latestVersionResult.isOk()
	                           && *latestVersionResult.ok() != CURRENT_VERSION);
	M5EPD_Canvas& c = getScreenBuffer();
	for (auto& p : _panels) p->drawToCanvas(c);
	for (auto& t : _texts) t->drawToCanvas(c);
	for (auto& b : _buttons) b->drawToCanvas(c);
	wakeDisplay();
	c.pushCanvas(0, 0, mode);
	sleepDisplay();
}

void SettingsScreen::handleTouch(int16_t x, int16_t y) {
	for (auto& b : _buttons) b->handleTouch(x, y);
}

}  // namespace gui