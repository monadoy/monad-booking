#include "mainScreen.h"

#include "globals.h"
#include "gui/displayUtils.h"

namespace gui {

MainScreen::MainScreen() {
	const int l_pnl_w = 652;
	const int l_txt_pad = 80;
	const int l_txt_w = l_pnl_w - 2 * l_txt_pad;

	const int r_pnl_w = 308;
	const int r_txt_pad = 32;
	const int r_txt_w = r_pnl_w - 2 * r_txt_pad;
	const int r_txt_pos = l_pnl_w + r_txt_pad;

	ADD_PNL(PNL_LEFT, Panel(Pos{0, 0}, Size{l_pnl_w, 540}, L_PNL));
	ADD_PNL(PNL_RIGHT, Panel(Pos{652, 0}, Size{308, 540}, R_PNL));

	ADD_TXT(TXT_TOP_CLOCK, Text(Pos{875, 20}, Size{77, 40}, "00:00", FS_NORMAL, BK, R_PNL));
	ADD_TXT(TXT_MID_CLOCK, Text(Pos{l_txt_pad, 92}, Size{77, 40}, "00:00", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TXT_ROOM_NAME,
	        Text(Pos{l_txt_pad, 125}, Size{l_txt_w, 40}, "room", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TXT_TITLE, Text(Pos{l_txt_pad, 164}, Size{l_txt_w, 77},
	                        l10n.msg(L10nMessage::NOT_BOOKED), FS_TITLE, BK, L_PNL, true));

	ADD_TXT(TXT_L_FREE_SUBHEADER,
	        Text(Pos{l_txt_pad, 249}, Size{l_txt_w, 60}, l10n.msg(L10nMessage::BOOK_ROOM),
	             FS_HEADER, BK, L_PNL, true));

	ADD_TXT(TXT_L_TAKEN_ORGANIZER, Text(Pos{l_txt_pad, 252}, Size{l_txt_w, 40},
	                                    "organizer@gmail.com", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(TXT_L_TAKEN_SUMMARY,
	        Text(Pos{l_txt_pad, 289}, Size{l_txt_w, 40}, "summary", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(TXT_L_TAKEN_TIMESPAN, Text(Pos{l_txt_pad, 330}, Size{l_txt_w, 53}, "00:00 - 00:00",
	                                   FS_TIMESPAN, WH, L_PNL_TAKEN, true));

	ADD_TXT(TXT_R_FREE_HEADER,
	        Text(Pos{r_txt_pos, 359}, Size{r_txt_w, 135}, l10n.msg(L10nMessage::NO_UPCOMING_EVENTS),
	             FS_HEADER, BK, R_PNL, true));

	ADD_TXT(TXT_R_TAKEN_HEADER,
	        Text(Pos{r_txt_pos, 161}, Size{r_txt_w, 135}, l10n.msg(L10nMessage::NEXT_EVENT),
	             FS_HEADER, BK, R_PNL_TAKEN, true));
	ADD_TXT(TXT_R_TAKEN_ORGANIZER, Text(Pos{r_txt_pos, 246}, Size{r_txt_w, 40},
	                                    "organizer@gmail.com", FS_NORMAL, BK, R_PNL_TAKEN));
	ADD_TXT(TXT_R_TAKEN_SUMMARY,
	        Text(Pos{r_txt_pos, 279}, Size{r_txt_w, 87}, "summary", FS_NORMAL, BK, R_PNL_TAKEN));
	ADD_TXT(TXT_R_TAKEN_TIMESPAN, Text(Pos{r_txt_pos, 370}, Size{r_txt_w, 106}, "00:00 -\n00:00",
	                                   FS_TIMESPAN, BK, R_PNL_TAKEN, true));

	ADD_BTN(BTN_SETTINGS, Button(Pos{15, 15}, Size{64, 56}, "/images/settingsWhite.png",
	                             [this]() { log_i("clicked settings"); }));
	ADD_BTN(BTN_15, Button(BTN_GRID_POSITIONS[0], Size{140, 78}, "15", FS_BUTTON, WH, BK,
	                       [this]() { onBook(15); }));
	ADD_BTN(BTN_30, Button(BTN_GRID_POSITIONS[1], Size{140, 78}, "30", FS_BUTTON, WH, BK,
	                       [this]() { onBook(30); }));
	ADD_BTN(BTN_60, Button(BTN_GRID_POSITIONS[2], Size{140, 78}, "60", FS_BUTTON, WH, BK,
	                       [this]() { onBook(60); }));
	ADD_BTN(BTN_90, Button(BTN_GRID_POSITIONS[3], Size{140, 78}, "90", FS_BUTTON, WH, BK,
	                       [this]() { onBook(90); }));
	ADD_BTN(BTN_UNTIL_NEXT,
	        Button(BTN_GRID_POSITIONS[4], Size{332, 78}, l10n.msg(L10nMessage::UNTIL_NEXT),
	               FS_BUTTON, WH, BK, [this]() { onBookUntilNext(); }));
	ADD_BTN(BTN_FREE_ROOM,
	        Button(BTN_GRID_POSITIONS[3], Size{291, 78}, l10n.msg(L10nMessage::FREE_ROOM),
	               FS_BUTTON, BK, WH, [this]() { onFree(); }));
	ADD_BTN(BTN_EXTEND_15, Button(BTN_GRID_POSITIONS[5], Size{140, 78}, "+15", FS_BUTTON, BK, WH,
	                              [this]() { onExtend(); }));

	ASSERT_ALL_ELEMENTS();
}

void MainScreen::update(std::shared_ptr<cal::CalendarStatus> status, bool doDraw) {
	if (status) {
		_status = status;

		show();

		// UPDATE TEXT CONTENTS
		_texts[TXT_ROOM_NAME]->setText(_status->name);
		_texts[TXT_TITLE]->setText(_status->currentEvent ? l10n.msg(L10nMessage::BOOKED)
		                                                 : l10n.msg(L10nMessage::NOT_BOOKED));
		if (_status->currentEvent) {
			auto ce = _status->currentEvent;
			_texts[TXT_L_TAKEN_ORGANIZER]->setText(ce->creator);
			_texts[TXT_L_TAKEN_SUMMARY]->setText(ce->summary);
			_texts[TXT_L_TAKEN_TIMESPAN]->setText(timeSpanStr(ce->unixStartTime, ce->unixEndTime));
		}
		_panels[PNL_RIGHT]->setColor(_status->nextEvent ? R_PNL_TAKEN : R_PNL);
		if (_status->nextEvent) {
			auto ne = _status->nextEvent;
			_texts[TXT_R_TAKEN_ORGANIZER]->setText(ne->creator);
			_texts[TXT_R_TAKEN_SUMMARY]->setText(ne->summary);
			String timeStr = safeMyTZ.dateTime(ne->unixStartTime, UTC_TIME, "G:i") + " -\n"
			                 + safeMyTZ.dateTime(ne->unixEndTime, UTC_TIME, "G:i");
			_texts[TXT_R_TAKEN_TIMESPAN]->setText(timeSpanStr(ne->unixStartTime, ne->unixEndTime));
		}

		// UPDATE LEFT PANEL COLORS
		uint8_t leftColor = _status->currentEvent ? WH : BK;
		uint8_t leftBGColor = _status->currentEvent ? L_PNL_TAKEN : L_PNL;
		_panels[PNL_LEFT]->setColor(leftBGColor);
		_texts[TXT_MID_CLOCK]->setColors(leftColor, leftBGColor);
		_texts[TXT_ROOM_NAME]->setColors(leftColor, leftBGColor);
		_texts[TXT_TITLE]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_FREE_SUBHEADER]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_TAKEN_ORGANIZER]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_TAKEN_SUMMARY]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_TAKEN_TIMESPAN]->setColors(leftColor, leftBGColor);
		_buttons[BTN_SETTINGS]->setReverseColor(_status->currentEvent ? true : false);

		// UPDATE RIGHT PANEL COLORS (text colors stay black, only bg changes)
		uint8_t rightBGColor = _status->nextEvent ? R_PNL_TAKEN : R_PNL;
		_panels[PNL_RIGHT]->setColor(rightBGColor);
		_texts[TXT_TOP_CLOCK]->setBGColor(rightBGColor);
		_texts[TXT_R_FREE_HEADER]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_HEADER]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_ORGANIZER]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_SUMMARY]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_TIMESPAN]->setBGColor(rightBGColor);

		_curBatteryStyle = _status->nextEvent ? BATTERY_DARKER : BATTERY_LIGHT;
	};

	_texts[TXT_TOP_CLOCK]->setText(safeMyTZ.dateTime("G:i"));
	_texts[TXT_MID_CLOCK]->setText(safeMyTZ.dateTime("G:i"));

	_curBatteryImage = utils::isCharging() ? 4 : uint8_t(utils::getBatteryLevel() * 3.9999);

	if (doDraw) {
		draw(UPDATE_MODE_GC16);
		// TODO: do a less flashy draw if possible (update clock, battery and buttons individually
		// if screen color hasn't changed)
	}
}

void MainScreen::show(bool doShow) {
	Screen::show(doShow);

	for (auto& p : _panels) p->show(doShow);
	for (auto& t : _texts) t->show(doShow);
	for (auto& b : _buttons) b->show(doShow);

	if (doShow) {
		// Hide specific things based on taken or free
		bool haveCurEvent = !!_status->currentEvent;
		bool haveNextEvent = !!_status->nextEvent;
		time_t diffToNext
		    = haveNextEvent ? _status->nextEvent->unixStartTime - safeUTC.now() : LONG_MAX;

		time_t diffFromCurrToNext
		    = haveCurEvent && haveNextEvent
		          ? _status->nextEvent->unixStartTime - _status->currentEvent->unixEndTime
		          : LONG_MAX;

		_texts[TXT_L_FREE_SUBHEADER]->show(!haveCurEvent);
		_texts[TXT_L_TAKEN_ORGANIZER]->show(haveCurEvent);
		_texts[TXT_L_TAKEN_SUMMARY]->show(haveCurEvent);
		_texts[TXT_L_TAKEN_TIMESPAN]->show(haveCurEvent);

		_texts[TXT_R_FREE_HEADER]->show(!haveNextEvent);

		_texts[TXT_R_TAKEN_HEADER]->show(haveNextEvent);
		_texts[TXT_R_TAKEN_ORGANIZER]->show(haveNextEvent);
		_texts[TXT_R_TAKEN_SUMMARY]->show(haveNextEvent);
		_texts[TXT_R_TAKEN_TIMESPAN]->show(haveNextEvent);

		// Booking buttons with numbers are pretty complicated, they need to be hidden based on
		// the current and next event. Also the "until next" button needs to be properly positioned
		// after the 90min button or to the left side.
		std::array<bool, 4> buttonShow{!haveCurEvent && diffToNext >= 15 * SECS_PER_MIN,
		                               !haveCurEvent && diffToNext >= 30 * SECS_PER_MIN,
		                               !haveCurEvent && diffToNext >= 60 * SECS_PER_MIN,
		                               !haveCurEvent && diffToNext >= 90 * SECS_PER_MIN};
		for (int i = 0; i < buttonShow.size(); i++) {
			if (!buttonShow[i])
				_buttons[BTN_15 + i]->hide();
		}
		_buttons[BTN_UNTIL_NEXT]->show(!haveCurEvent && haveNextEvent
		                               && diffToNext >= 5 * SECS_PER_MIN
		                               && diffToNext <= 3 * 60 * SECS_PER_MIN);
		// Move the "until next" button to the third or fourth position depending on the 90 button
		_buttons[BTN_UNTIL_NEXT]->setPos(BTN_GRID_POSITIONS[3 + size_t(buttonShow[3])]);

		_buttons[BTN_FREE_ROOM]->show(haveCurEvent);
		_buttons[BTN_EXTEND_15]->show(haveCurEvent && diffFromCurrToNext >= 15 * SECS_PER_MIN);
	}
}

void MainScreen::draw(m5epd_update_mode_t mode) {
	M5.EPD.Active();
	for (auto& p : _panels) p->draw(UPDATE_MODE_NONE);
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	for (auto& b : _buttons) b->draw(UPDATE_MODE_NONE);
	_batteryAnim[_curBatteryStyle].drawFrame(_curBatteryImage + 1, UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(mode);
}

void MainScreen::handleTouch(int16_t x, int16_t y) {
	for (auto& b : _buttons) b->handleTouch(x, y);
}

}  // namespace gui