#include "mainScreen.h"

#include "globals.h"

namespace gui {

MainScreen::MainScreen() {
	ADD_PNL(PNL_LEFT, Panel(Pos{0, 0}, Size{652, 540}, L_PNL));
	ADD_PNL(PNL_RIGHT, Panel(Pos{652, 0}, Size{308, 540}, R_PNL));

	ADD_TXT(TXT_TOP_CLOCK, Text(Pos{875, 20}, Size{77, 40}, "00:00", FS_NORMAL, BK, R_PNL));
	ADD_TXT(TXT_MID_CLOCK, Text(Pos{80, 92}, Size{77, 40}, "00:00", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TXT_ROOM_NAME, Text(Pos{80, 125}, Size{652 - 90, 40}, "room", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TXT_HEADER, Text(Pos{80, 164}, Size{418, 77}, l10n.msg(L10nMessage::NOT_BOOKED),
	                         FS_TITLE, BK, L_PNL, true));

	ADD_TXT(TXT_L_FREE_SUBHEADER,
	        Text(Pos{80, 249}, Size{300, 60}, l10n.msg(L10nMessage::BOOK_ROOM), FS_HEADER, BK,
	             L_PNL, true));

	ADD_TXT(TXT_L_TAKEN_ORGANIZER, Text(Pos{80, 252}, Size{652 - 90, 40}, "organizer@gmail.com",
	                                    FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(TXT_L_TAKEN_SUMMARY,
	        Text(Pos{80, 289}, Size{652 - 90, 40}, "summary", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(TXT_L_TAKEN_TIMESPAN,
	        Text(Pos{80, 330}, Size{412, 53}, "00:00 - 00:00", FS_TIMESPAN, WH, L_PNL_TAKEN, true));
	ADD_TXT(TXT_R_FREE_HEADER,
	        Text(Pos{701, 359}, Size{231, 135}, l10n.msg(L10nMessage::NO_UPCOMING_EVENTS),
	             FS_HEADER, BK, R_PNL, true));

	ADD_TXT(TXT_R_TAKEN_HEADER,
	        Text(Pos{701, 161}, Size{231, 135}, l10n.msg(L10nMessage::NEXT_EVENT), FS_HEADER, BK,
	             R_PNL_TAKEN, true));
	ADD_TXT(TXT_R_TAKEN_ORGANIZER,
	        Text(Pos{701, 246}, Size{239, 40}, "organizer@gmail.com", FS_NORMAL, BK, R_PNL_TAKEN));
	ADD_TXT(TXT_R_TAKEN_SUMMARY,
	        Text(Pos{701, 279}, Size{231, 87}, "summary", FS_NORMAL, BK, R_PNL_TAKEN));

	ADD_TXT(TXT_R_TAKEN_TIMESPAN, Text(Pos{701, 370}, Size{231, 106}, "00:00 -\n00:00", FS_TIMESPAN,
	                                   BK, R_PNL_TAKEN, true));

	ASSERT_ALL_ELEMENTS();
}

void MainScreen::update(std::shared_ptr<cal::CalendarStatus> status, bool doDraw) {
	log_i("Setting calendar status2");
	if (status) {
		_status = status;

		// UPDATE TEXT CONTENTS
		_texts[TXT_ROOM_NAME]->setText(_status->name);
		if (_status->currentEvent) {
			auto ce = _status->currentEvent;
			_texts[TXT_L_TAKEN_ORGANIZER]->setText(ce->creator);
			_texts[TXT_L_TAKEN_SUMMARY]->setText(ce->summary);
			_texts[TXT_L_TAKEN_TIMESPAN]->setText(
			    safeMyTZ.dateTime(ce->unixStartTime, UTC_TIME, "G:i") + " - "
			    + safeMyTZ.dateTime(ce->unixEndTime, UTC_TIME, "G:i"));
		}
		_panels[PNL_RIGHT]->setColor(_status->nextEvent ? R_PNL_TAKEN : R_PNL);
		if (_status->nextEvent) {
			auto ne = _status->nextEvent;
			_texts[TXT_R_TAKEN_ORGANIZER]->setText(ne->creator);
			_texts[TXT_R_TAKEN_SUMMARY]->setText(ne->summary);
			_texts[TXT_R_TAKEN_TIMESPAN]->setText(
			    safeMyTZ.dateTime(ne->unixStartTime, UTC_TIME, "G:i") + " -\n"
			    + safeMyTZ.dateTime(ne->unixEndTime, UTC_TIME, "G:i"));
		}

		// UPDATE LEFT PANEL COLORS
		uint8_t leftColor = _status->currentEvent ? WH : BK;
		uint8_t leftBGColor = _status->currentEvent ? L_PNL_TAKEN : L_PNL;
		_panels[PNL_LEFT]->setColor(leftBGColor);
		_texts[TXT_MID_CLOCK]->setColors(leftColor, leftBGColor);
		_texts[TXT_ROOM_NAME]->setColors(leftColor, leftBGColor);
		_texts[TXT_HEADER]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_FREE_SUBHEADER]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_TAKEN_ORGANIZER]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_TAKEN_SUMMARY]->setColors(leftColor, leftBGColor);
		_texts[TXT_L_TAKEN_TIMESPAN]->setColors(leftColor, leftBGColor);

		// UPDATE RIGHT PANEL COLORS (text colors stay black, only bg changes)
		uint8_t rightBGColor = _status->nextEvent ? R_PNL_TAKEN : R_PNL;
		_panels[PNL_RIGHT]->setColor(rightBGColor);
		_texts[TXT_TOP_CLOCK]->setBGColor(rightBGColor);
		_texts[TXT_R_FREE_HEADER]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_HEADER]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_ORGANIZER]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_SUMMARY]->setBGColor(rightBGColor);
		_texts[TXT_R_TAKEN_TIMESPAN]->setBGColor(rightBGColor);

		show();
	};

	_texts[TXT_TOP_CLOCK]->setText(safeMyTZ.dateTime("G:i"));
	_texts[TXT_MID_CLOCK]->setText(safeMyTZ.dateTime("G:i"));

	_curBatteryImage = utils::isCharging() ? 4 : uint8_t(utils::getBatteryLevel() * 3.9999);

	if (doDraw) {
		if (status)
			draw(UPDATE_MODE_GC16);
		else
			draw(UPDATE_MODE_GC16);  // TODO: Do a lighter draw if status hasn't changed
	}
}

void MainScreen::show(bool show) {
	Screen::show(show);

	for (auto& p : _panels) p->show(show);
	for (auto& t : _texts) t->show(show);
	for (auto& b : _buttons) b->show(show);

	if (show) {
		// Hide specific things based on taken or free
		bool haveCurEvent = _status->currentEvent != nullptr;
		_texts[TXT_L_FREE_SUBHEADER]->show(!haveCurEvent);
		_texts[TXT_L_TAKEN_ORGANIZER]->show(haveCurEvent);
		_texts[TXT_L_TAKEN_SUMMARY]->show(haveCurEvent);
		_texts[TXT_L_TAKEN_TIMESPAN]->show(haveCurEvent);

		bool haveNextEvent = _status->nextEvent != nullptr;
		_texts[TXT_R_FREE_HEADER]->show(!haveNextEvent);
		_texts[TXT_R_TAKEN_HEADER]->show(haveNextEvent);
		_texts[TXT_R_TAKEN_ORGANIZER]->show(haveNextEvent);
		_texts[TXT_R_TAKEN_SUMMARY]->show(haveNextEvent);
		_texts[TXT_R_TAKEN_TIMESPAN]->show(haveNextEvent);
	}
}

void MainScreen::draw(m5epd_update_mode_t mode) {
	for (auto& p : _panels) p->draw(UPDATE_MODE_NONE);
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	for (auto& b : _buttons) b->draw(UPDATE_MODE_NONE);
	_batteryAnim.drawFrame(_curBatteryImage, UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(mode);
}

void MainScreen::handleTouch(int16_t x, int16_t y) {
	for (auto& b : _buttons) b->handleTouch(x, y);
}

}  // namespace gui