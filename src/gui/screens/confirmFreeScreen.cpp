#include "confirmFreeScreen.h"

#include "globals.h"
#include "gui/displayUtils.h"

namespace gui {

ConfirmFreeScreen::ConfirmFreeScreen() {
	const int txt_pad = 144;
	const int txt_w = 960 - 2 * txt_pad;

	ADD_PNL(PNL_MAIN, Panel(Pos{0, 0}, Size{960, 540}, WH));

	ADD_TXT(TXT_TITLE, Text(Pos{txt_pad, 164}, Size{txt_w, 40},
	                        l10n.msg(L10nMessage::FREE_CONFIRM_TITLE), FS_HEADER, BK, WH, true));
	ADD_TXT(TXT_ORGANIZER, Text(Pos{txt_pad, 164 + 40 + 12}, Size{txt_w, 32},
	                            l10n.msg(L10nMessage::NEW_EVENT_SUMMARY), FS_NORMAL, BK, WH));
	ADD_TXT(TXT_SUMMARY, Text(Pos{txt_pad, 164 + 40 + 12 + 32 + 8}, Size{txt_w, 32},
	                          l10n.msg(L10nMessage::NEW_EVENT_SUMMARY), FS_NORMAL, BK, WH));
	ADD_TXT(TXT_TIMESPAN, Text(Pos{txt_pad, 164 + 40 + 12 + 32 + 8 + 32 + 12}, Size{txt_w, 52},
	                           "00:00 - 00:00", FS_TIMESPAN, BK, WH, true));

	ADD_BTN(BTN_CANCEL, Button(Pos{420, 400}, Size{200, 78}, l10n.msg(L10nMessage::CANCEL),
	                           FS_BUTTON, BK, WH, [this]() { onCancel(); }));
	ADD_BTN(BTN_CONFIRM,
	        Button(Pos{420 + 200 + 12, 400}, Size{200, 78}, l10n.msg(L10nMessage::FREE), FS_BUTTON,
	               WH, BK, [this]() { onConfirm(); }));

	ASSERT_ALL_ELEMENTS();
}

void ConfirmFreeScreen::showEvent(std::shared_ptr<cal::Event> event) {
	_reserveParams = event;
	_texts[TXT_ORGANIZER]->setText(event->creator);
	_texts[TXT_SUMMARY]->setText(event->summary);
	_texts[TXT_TIMESPAN]->setText(timeSpanStr(event->unixStartTime, event->unixEndTime));
}

void ConfirmFreeScreen::show(bool doShow) {
	Screen::show(doShow);

	for (auto& p : _panels) p->show(doShow);
	for (auto& t : _texts) t->show(doShow);
	for (auto& b : _buttons) b->show(doShow);
}

void ConfirmFreeScreen::draw(m5epd_update_mode_t mode) {
	M5.EPD.Active();
	for (auto& p : _panels) p->draw(UPDATE_MODE_NONE);
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	for (auto& b : _buttons) b->draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(mode);
}

void ConfirmFreeScreen::handleTouch(int16_t x, int16_t y) {
	for (auto& b : _buttons) b->handleTouch(x, y);
}

}  // namespace gui