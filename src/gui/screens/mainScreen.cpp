#include "mainScreen.h"

#include "globals.h"

namespace gui {

#define ADD_PNL(id, p) _panels[size_t(id)] = std::unique_ptr<Panel>(p)
#define ADD_TXT(id, t) _texts[size_t(id)] = std::unique_ptr<Text>(t)
#define ADD_BTN(id, b) _buttons[size_t(id)] = std::unique_ptr<Button>(b)

MainScreen::MainScreen() {
	ADD_PNL(PanelId::LEFT, new Panel(Pos{0, 0}, Size{652, 540}, L_PNL));
	ADD_PNL(PanelId::RIGHT, new Panel(Pos{653, 0}, Size{308, 540}, R_PNL));

	ADD_TXT(TextId::MID_CLOCK, new Text(Pos{80, 92}, Size{77, 40}, "00:00", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TextId::ROOM_NAME,
	        new Text(Pos{80, 125}, Size{652 - 90, 40}, "room", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TextId::HEADER, new Text(Pos{80, 164}, Size{418, 77}, l10n.msg(L10nMessage::NOT_BOOKED),
	                                 FS_TITLE, BK, L_PNL, true));

	ADD_TXT(TextId::L_FREE_SUBHEADER,
	        new Text(Pos{80, 249}, Size{300, 60}, l10n.msg(L10nMessage::BOOK_ROOM), FS_HEADER, BK,
	                 L_PNL, true));

	ADD_TXT(TextId::L_TAKEN_ORGANIZER, new Text(Pos{80, 252}, Size{652 - 90, 40},
	                                            "organizer@gmail.com", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(TextId::L_TAKEN_SUMMARY,
	        new Text(Pos{80, 289}, Size{652 - 90, 40}, "summary", FS_NORMAL, WH, L_PNL_TAKEN));
	ADD_TXT(TextId::L_TAKEN_TIMESPAN, new Text(Pos{80, 330}, Size{412, 53}, "00:00 - 00:00",
	                                           FS_TIMESPAN, WH, L_PNL_TAKEN, true));
	ADD_TXT(TextId::R_FREE_HEADER,
	        new Text(Pos{701, 359}, Size{231, 135}, l10n.msg(L10nMessage::NO_UPCOMING_EVENTS),
	                 FS_HEADER, BK, R_PNL, true));

	ADD_TXT(TextId::R_TAKEN_HEADER,
	        new Text(Pos{701, 161}, Size{231, 135}, l10n.msg(L10nMessage::NEXT_EVENT), FS_HEADER,
	                 BK, R_PNL_TAKEN, true));
	ADD_TXT(TextId::R_TAKEN_ORGANIZER, new Text(Pos{701, 246}, Size{239, 40}, "organizer@gmail.com",
	                                            FS_NORMAL, BK, R_PNL_TAKEN));
	ADD_TXT(TextId::R_TAKEN_SUMMARY,
	        new Text(Pos{701, 279}, Size{231, 87}, "summary", FS_NORMAL, BK, R_PNL_TAKEN));

	ADD_TXT(TextId::R_TAKEN_TIMESPAN, new Text(Pos{701, 370}, Size{231, 106}, "00:00 -\n00:00",
	                                           FS_TIMESPAN, BK, R_PNL_TAKEN, true));
}

void MainScreen::update(std::shared_ptr<cal::CalendarStatus> status) {
	if (status) {
		_status = status;
		_texts[size_t(TextId::ROOM_NAME)]->setText(_status->name);
		if (_status->currentEvent) {
			auto ce = _status->currentEvent;
			_texts[size_t(TextId::L_TAKEN_ORGANIZER)]->setText(ce->creator);
			_texts[size_t(TextId::L_TAKEN_SUMMARY)]->setText(ce->summary);
			_texts[size_t(TextId::L_TAKEN_TIMESPAN)]->setText(
			    safeMyTZ.dateTime(ce->unixStartTime, UTC_TIME, "G:i") + " - "
			    + safeMyTZ.dateTime(ce->unixEndTime, UTC_TIME, "G:i"));
		}
		if (_status->nextEvent) {
			auto ne = _status->nextEvent;
			_texts[size_t(TextId::R_TAKEN_ORGANIZER)]->setText(ne->creator);
			_texts[size_t(TextId::R_TAKEN_SUMMARY)]->setText(ne->summary);
			_texts[size_t(TextId::R_TAKEN_TIMESPAN)]->setText(
			    safeMyTZ.dateTime(ne->unixStartTime, UTC_TIME, "G:i") + " -\n"
			    + safeMyTZ.dateTime(ne->unixEndTime, UTC_TIME, "G:i"));
		}
	};

	_texts[size_t(TextId::TOP_CLOCK)]->setText(safeMyTZ.dateTime("G:i"));
	_texts[size_t(TextId::MID_CLOCK)]->setText(safeMyTZ.dateTime("G:i"));

	_curBatteryImage = utils::isCharging() ? 4 : uint8_t(utils::getBatteryLevel() * 3.9999);

	if (status)
		draw(UPDATE_MODE_GC16);
	else
		draw(UPDATE_MODE_GC16);  // TODO: Do a lighter draw if status hasn't changed
}

void MainScreen::show(bool show) {
	Screen::show(show);

	for (auto& p : _panels) p->show(show);
	for (auto& t : _texts) t->show(show);
	for (auto& b : _buttons) b->show(show);

	if (show) {
		// Hide specific things based on taken or free
		bool haveCurEvent = _status->currentEvent != nullptr;
		_texts[size_t(TextId::L_FREE_SUBHEADER)]->show(!haveCurEvent);
		_texts[size_t(TextId::L_TAKEN_ORGANIZER)]->show(haveCurEvent);
		_texts[size_t(TextId::L_TAKEN_SUMMARY)]->show(haveCurEvent);
		_texts[size_t(TextId::L_TAKEN_TIMESPAN)]->show(haveCurEvent);

		bool haveNextEvent = _status->nextEvent != nullptr;
		_texts[size_t(TextId::R_FREE_HEADER)]->show(!haveNextEvent);
		_texts[size_t(TextId::R_TAKEN_HEADER)]->show(haveNextEvent);
		_texts[size_t(TextId::R_TAKEN_ORGANIZER)]->show(haveNextEvent);
		_texts[size_t(TextId::R_TAKEN_SUMMARY)]->show(haveNextEvent);
		_texts[size_t(TextId::R_TAKEN_TIMESPAN)]->show(haveNextEvent);
	}
}

void MainScreen::draw(m5epd_update_mode_t mode) {
	for (auto& p : _panels) p->draw(UPDATE_MODE_NONE);
	for (auto& t : _texts) t->draw(UPDATE_MODE_NONE);
	for (auto& b : _buttons) b->draw(UPDATE_MODE_NONE);
	_batteryAnim.drawFrame(_curBatteryImage, UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(mode);
}

}  // namespace gui