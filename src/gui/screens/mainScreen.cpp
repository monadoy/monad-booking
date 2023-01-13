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

	ADD_TXT(TXT_TOP_CLOCK,
	        Text(Pos{872, 20}, Size{74, 30}, "00:00", FS_NORMAL, BK, R_PNL, false, Align::RIGHT));
	ADD_TXT(TXT_BATTERY_LEVEL,
	        Text(Pos{748, 20}, Size{58, 30}, "100", FS_NORMAL, BK, R_PNL, false, Align::RIGHT));
	ADD_TXT(TXT_MID_CLOCK, Text(Pos{l_txt_pad, 92}, Size{70, 30}, "00:00", FS_NORMAL, BK, L_PNL));
	ADD_TXT(TXT_ROOM_NAME, Text(Pos{l_txt_pad, 125}, Size{l_txt_w, 40}, "", FS_NORMAL, BK, L_PNL));
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

	ADD_TXT(TXT_ERROR, Text(Pos{156, 8}, Size{l_pnl_w - 156 - 16, 112}, "", FS_NORMAL, BK, 1));

	ADD_TXT(TXT_BATTERY_WARNING,
	        Text(Pos{l_pnl_w + r_txt_pad, 64}, Size{r_txt_w, 56}, l10n.msg(L10nMessage::CHARGE_ME),
	             FS_NORMAL, WH, 14, false, Align::LEFT, Margins{16, 16, 16, 80}));

	ADD_BTN(BTN_SETTINGS, Button(Pos{16, 16}, Size{64, 56}, "/images/settingsWhite.png",
	                             [this]() { onGoSettings(); }));
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

	// Set initial screen state (hides and shows correct stuff)
	setStatus(std::shared_ptr<cal::CalendarStatus>(new cal::CalendarStatus{
	    .name = "Couldn't fetch room name", .currentEvent = nullptr, .nextEvent = nullptr}));
}
void MainScreen::_updateLeftSide() {
	std::shared_ptr<cal::Event>& event = _status->currentEvent;
	bool taken = !!event;

	// Update text
	_texts[TXT_ROOM_NAME]->setText(_status->name);
	_texts[TXT_TITLE]->setText(taken ? l10n.msg(L10nMessage::BOOKED)
	                                 : l10n.msg(L10nMessage::NOT_BOOKED));
	if (taken) {
		_texts[TXT_L_TAKEN_ORGANIZER]->setText(event->creator);
		_texts[TXT_L_TAKEN_SUMMARY]->setText(event->summary);
		_texts[TXT_L_TAKEN_TIMESPAN]->setText(
		    timeSpanStr(event->unixStartTime, event->unixEndTime));
	}

	// Hide/show
	_texts[TXT_L_FREE_SUBHEADER]->show(!taken);
	_texts[TXT_L_TAKEN_ORGANIZER]->show(taken);
	_texts[TXT_L_TAKEN_SUMMARY]->show(taken);
	_texts[TXT_L_TAKEN_TIMESPAN]->show(taken);

	// Update colors
	uint8_t color = taken ? WH : BK;
	uint8_t bgColor = taken ? L_PNL_TAKEN : L_PNL;
	_panels[PNL_LEFT]->setColor(bgColor);
	_texts[TXT_MID_CLOCK]->setColors(color, bgColor);
	_texts[TXT_ROOM_NAME]->setColors(color, bgColor);
	_texts[TXT_TITLE]->setColors(color, bgColor);
	_texts[TXT_L_FREE_SUBHEADER]->setColors(color, bgColor);
	_texts[TXT_L_TAKEN_ORGANIZER]->setColors(color, bgColor);
	_texts[TXT_L_TAKEN_SUMMARY]->setColors(color, bgColor);
	_texts[TXT_L_TAKEN_TIMESPAN]->setColors(color, bgColor);
	_buttons[BTN_SETTINGS]->setReverseColor(taken ? true : false);
}

void MainScreen::_updateRightSide() {
	std::shared_ptr<cal::Event>& event = _status->nextEvent;
	bool taken = !!event;

	// Update text
	if (event) {
		_texts[TXT_R_TAKEN_ORGANIZER]->setText(event->creator);
		_texts[TXT_R_TAKEN_SUMMARY]->setText(event->summary);
		_texts[TXT_R_TAKEN_TIMESPAN]->setText(
		    safeMyTZ.dateTime(event->unixStartTime, UTC_TIME, "G:i") + " -\n"
		    + safeMyTZ.dateTime(event->unixEndTime, UTC_TIME, "G:i"));
	}

	// Hide/show
	_texts[TXT_R_FREE_HEADER]->show(!taken);
	_texts[TXT_R_TAKEN_HEADER]->show(taken);
	_texts[TXT_R_TAKEN_ORGANIZER]->show(taken);
	_texts[TXT_R_TAKEN_SUMMARY]->show(taken);
	_texts[TXT_R_TAKEN_TIMESPAN]->show(taken);

	// Update colors (text colors stay black, only bg changes)
	uint8_t bgColor = taken ? R_PNL_TAKEN : R_PNL;
	_panels[PNL_RIGHT]->setColor(bgColor);
	_texts[TXT_TOP_CLOCK]->setBGColor(bgColor);
	_texts[TXT_BATTERY_LEVEL]->setBGColor(bgColor);
	_texts[TXT_R_FREE_HEADER]->setBGColor(bgColor);
	_texts[TXT_R_TAKEN_HEADER]->setBGColor(bgColor);
	_texts[TXT_R_TAKEN_ORGANIZER]->setBGColor(bgColor);
	_texts[TXT_R_TAKEN_SUMMARY]->setBGColor(bgColor);
	_texts[TXT_R_TAKEN_TIMESPAN]->setBGColor(bgColor);

	_batteryStyle = taken ? BATTERY_DARKER : BATTERY_LIGHT;
}

void MainScreen::setStatus(std::shared_ptr<cal::CalendarStatus> status) {
	log_i("setting status....");
	// status is  null if it hasn't changed
	if (status) {
		_statusChanged = true;
		_status = status;

		_updateLeftSide();
		_updateRightSide();
	}

	// Calling setStatus means that no error happened
	if (!_texts[TXT_ERROR]->isHidden()) {
		_errorChanged = true;
		_texts[TXT_ERROR]->hide();
	}
}

void MainScreen::setError(const String& error) {
	_errorChanged = true;
	_error = error;
	_texts[TXT_ERROR]->show();
	_texts[TXT_ERROR]->setText(_error);
}

void MainScreen::draw(m5epd_update_mode_t mode) { _drawImpl(mode, false); }

void MainScreen::reducedDraw(m5epd_update_mode_t mode) { _drawImpl(mode, true); }

const Pos POS_1 = Pos{.x = 748, .y = 20};
const Size SIZE_1 = Size{.w = 200, .h = 30};
uint8_t* part1_buf = new uint8_t[SIZE_1.w * SIZE_1.h / 2];

const Pos POS_2 = Pos{.x = 80, .y = 92};
const Size SIZE_2 = Size{.w = 72, .h = 30};
uint8_t* part2_buf = new uint8_t[SIZE_2.w * SIZE_2.h / 2];

void MainScreen::_drawImpl(m5epd_update_mode_t mode, bool allowReducedDraw) {
	// Always update elements that are expected to change all the time
	// (things based on clock and battery level)
	const String time = safeMyTZ.dateTime("G:i");
	_texts[TXT_TOP_CLOCK]->setText(time);
	_texts[TXT_MID_CLOCK]->setText(time);
	// Negative battery level means that we are charging
	float oldBatteryLevel = _batteryLevel;
	_batteryLevel = utils::getBatteryLevel();

	uint8_t oldBatteryImage = _batteryImage;
	_batteryImage = utils::isCharging() ? 4 : uint8_t(_batteryLevel * 3.9999);

	_texts[TXT_BATTERY_WARNING]->show(_batteryImage == 0);
	bool batteryWarningChanged = (oldBatteryImage == 0) != (_batteryImage == 0);

	_texts[TXT_BATTERY_LEVEL]->show(_batteryLevel >= 0);
	_texts[TXT_BATTERY_LEVEL]->setText(String(int(_batteryLevel * 100)));

	bool buttonsChanged = _updateButtons();

	log_d(
	    "Allow reduced draw: %d, status changed: %d, error changed: %d, battery warning changed: "
	    "%d, buttons changed: %d",
	    allowReducedDraw, _statusChanged, _errorChanged, batteryWarningChanged, buttonsChanged);

	M5EPD_Canvas& c = getScreenBuffer();
	// Do a special non flashy update if clock and battery level are the only things that
	// changed since last update
	if (allowReducedDraw && !_statusChanged && !_errorChanged && !batteryWarningChanged
	    && !buttonsChanged) {
		_texts[TXT_TOP_CLOCK]->drawToCanvas(c);
		if (oldBatteryImage != _batteryImage)
			_batteryAnim[_batteryStyle].drawFrameToCanvas(_batteryImage + 1, c);
		if (oldBatteryLevel != _batteryLevel)
			_texts[TXT_BATTERY_LEVEL]->drawToCanvas(c);
		_texts[TXT_MID_CLOCK]->drawToCanvas(c);

		readPartFromCanvas(POS_1, SIZE_1, c, M5EPD_PANEL_W, part1_buf);
		readPartFromCanvas(POS_2, SIZE_2, c, M5EPD_PANEL_W, part2_buf);

		wakeDisplay();
		// Update top right part with battery level and clock
		M5.EPD.WritePartGram4bpp(POS_1.x, POS_1.y, SIZE_1.w, SIZE_1.h, part1_buf);
		M5.EPD.UpdateArea(POS_1.x, POS_1.y, SIZE_1.w, SIZE_1.h, mode);

		// Update clock on left side
		M5.EPD.WritePartGram4bpp(POS_2.x, POS_2.y, SIZE_2.w, SIZE_2.h, part2_buf);
		M5.EPD.UpdateArea(POS_2.x, POS_2.y, SIZE_2.w, SIZE_2.h, mode);
		sleepDisplay();
	} else {
		for (auto& p : _panels) p->drawToCanvas(c);
		for (auto& t : _texts) t->drawToCanvas(c);
		for (auto& b : _buttons) b->drawToCanvas(c);
		_batteryAnim[_batteryStyle].drawFrameToCanvas(_batteryImage + 1, c);
		if (_batteryImage == 0) {
			_batteryWarningIcon.drawToCanvas(c);
		}
		wakeDisplay();
		c.pushCanvas(0, 0, mode);
		sleepDisplay();
	}

	_statusChanged = false;
	_errorChanged = false;
}

bool MainScreen::_updateButtons() {
	// Booking buttons are pretty complicated, they need to be hidden based on
	// the the timings of current and next event. Also the "until next" button needs to be
	// properly positioned after the 90min button or to the left side.

	bool curTaken = !!_status->currentEvent;
	bool nextTaken = !!_status->nextEvent;

	time_t diffToNext = nextTaken ? _status->nextEvent->unixStartTime - safeUTC.now() : LONG_MAX;
	time_t diffFromCurrToNext = curTaken && nextTaken ? _status->nextEvent->unixStartTime
	                                                        - _status->currentEvent->unixEndTime
	                                                  : LONG_MAX;

	std::array<bool, 5> oldButtonShow{!_buttons[BTN_15]->isHidden(), !_buttons[BTN_30]->isHidden(),
	                                  !_buttons[BTN_60]->isHidden(), !_buttons[BTN_90]->isHidden(),
	                                  !_buttons[BTN_UNTIL_NEXT]->isHidden()};

	std::array<bool, 5> buttonShow{
	    !curTaken && diffToNext >= 15 * SECS_PER_MIN,  // 15min
	    !curTaken && diffToNext >= 30 * SECS_PER_MIN,  // 30min
	    !curTaken && diffToNext >= 60 * SECS_PER_MIN,  // 60min
	    !curTaken && diffToNext >= 90 * SECS_PER_MIN,  // 90min
	    !curTaken && nextTaken && diffToNext >= 5 * SECS_PER_MIN
	        && diffToNext <= 3 * 60 * SECS_PER_MIN  // "until next"
	};
	for (int i = 0; i < buttonShow.size(); i++) {
		_buttons[BTN_15 + i]->show(buttonShow[i]);
	}
	// Move the "until next" button to the third or fourth position depending on the "90" button
	_buttons[BTN_UNTIL_NEXT]->setPos(BTN_GRID_POSITIONS[3 + size_t(buttonShow[3])]);

	_buttons[BTN_FREE_ROOM]->show(curTaken);
	_buttons[BTN_EXTEND_15]->show(curTaken && diffFromCurrToNext >= 15 * SECS_PER_MIN);

	bool changed = false;
	for (int i = 0; i < buttonShow.size(); i++) {
		changed |= oldButtonShow[i] != buttonShow[i];
	}
	return changed;
}

void MainScreen::handleTouch(int16_t x, int16_t y) {
	for (auto& b : _buttons) b->handleTouch(x, y);
}

}  // namespace gui