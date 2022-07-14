#include "gui/gui.h"

#include <WiFi.h>

#include "animManager.h"
#include "configServer.h"
#include "globals.h"
#include "localization.h"
#include "myUpdate.h"
#include "safeTimezone.h"

namespace {

String interBold = "/interbold.ttf";
String interRegular = "/interregular.ttf";

EPDGUI_Button* btns[BUTTON_SIZE];
EPDGUI_Textbox* lbls[LABEL_SIZE];
M5EPD_Canvas canvasCurrentEvent(&M5.EPD);
M5EPD_Canvas canvasNextEvent(&M5.EPD);

std::shared_ptr<cal::Event> currentEvent = nullptr;
std::shared_ptr<cal::Event> nextEvent = nullptr;
cal::Model* _model = nullptr;
gui::GUITask* _guiTask = nullptr;
std::unique_ptr<anim::Animation> _animation = nullptr;
std::unique_ptr<Config::ConfigServer> configServer = nullptr;
std::unique_ptr<Config::ConfigStore> _configStore = nullptr;

cal::Token token;
String calendarId = "";
String resourceName = "";
uint16_t timeToBeBooked = 15;

const uint32_t BAT_LOW = 3300;
const uint32_t BAT_HIGH = 4200;
const uint16_t TEXT_PADDING_X = 48;
const uint16_t PIXELS_PER_LETTER_AVG = 16;

uint16_t currentScreen = SCREEN_MAIN;
uint16_t currentBtnIndex = 4;

std::vector<EPDGUI_Base*> epdgui_object_list;
uint32_t obj_id = 1;

bool needToPutSleep = true;
bool tillNext = false;
bool requireConfirmation = false;
std::shared_ptr<cal::Model::ReserveParams> reserveParamsPtr = nullptr;
std::atomic_bool gotResponse = ATOMIC_VAR_INIT(false);

std::vector<std::pair<int, int>> button_positions
    = {{80, 306}, {223, 306}, {371, 306}, {80, 399}, {223, 399}};

void EPDGUI_AddObject(EPDGUI_Base* object) {
	obj_id++;
	epdgui_object_list.push_back(object);
}

void EPDGUI_Draw(m5epd_update_mode_t mode) {
	for (std::vector<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		(*p)->Draw(mode);
	}
}

void EPDGUI_Draw(EPDGUI_Base* object, m5epd_update_mode_t mode) { object->Draw(mode); }

void EPDGUI_Process(void) {
	log_d("Finger lifted");
	bool isGoingToSleep = false;
	for (std::vector<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		bool screenGoingToUpdate = (*p)->UpdateState(-1, -1);
		if (screenGoingToUpdate) {
			isGoingToSleep = true;
		}
	}
	needToPutSleep = !isGoingToSleep;
	tryToPutSleep();
}

void EPDGUI_Process(int16_t x, int16_t y) {
	M5.EPD.Active();
	log_d("Touch at coordinates %u - %u", x, y);
	sleepManager.refreshTouchWake();
	bool isGoingToSleep = false;
	for (std::vector<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		bool screenGoingToUpdate = (*p)->UpdateState(x, y);
		if (screenGoingToUpdate) {
			isGoingToSleep = true;
		}
	}
}

String getBatteryPercent() {
	if (utils::isCharging()) {
		return "USB";
	}
	auto clamped = std::min(std::max(M5.getBatteryVoltage(), BAT_LOW), BAT_HIGH);
	int perc = (float)(clamped - BAT_LOW) / (float)(BAT_HIGH - BAT_LOW) * 100.0f;
	return String(perc) + "%";
}

bool checkEventEquality(std::shared_ptr<cal::Event> event1, std::shared_ptr<cal::Event> event2) {
	if (!!event1 != !!event2) {
		return false;
	}
	if (!event1 && !event2) {
		return true;
	}
	if (*event1 == *event2) {
		return true;
	}
	return false;
}

void updateStatus(std::shared_ptr<cal::CalendarStatus> statusCopy) {
	bool leftEventsEqual = false;
	bool updateRight = false;
	if (statusCopy) {
		bool leftEventsEqual = checkEventEquality(currentEvent, statusCopy->currentEvent);
		bool updateRight = checkEventEquality(nextEvent, statusCopy->nextEvent);
		resourceName = statusCopy->name;
		nextEvent = statusCopy->nextEvent;
		currentEvent = statusCopy->currentEvent;
	}
	int newBtnIndex = configureMainButtonPos(false);
	bool buttonsEqual = currentBtnIndex == newBtnIndex;
	bool updateLeft = leftEventsEqual && buttonsEqual;
	currentBtnIndex = newBtnIndex;
	if (currentScreen == SCREEN_MAIN) {
		toMainScreen(!updateLeft, !updateRight);
	} else
		M5.EPD.Sleep();
}

void updateScreen(bool pushLeft, bool pushRight, m5epd_update_mode_t updateMode) {
	if (pushLeft) {
		canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_NONE);
	}
	if (pushRight) {
		canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_NONE);
	}
	EPDGUI_Draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(updateMode);
}

// hides the next event on the right side
void hideNextBooking(bool isHide) {
	if (isHide) {
		canvasNextEvent.fillCanvas(0);
	} else {
		canvasNextEvent.fillCanvas(3);
	}
	// top bar
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->SetHide(isHide);
	}
	// the event
	for (int i = LABEL_NEXT_EVENT; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(isHide);
	}
}

int configureMainButtonPos(bool isHide) {
	int btnIndex = 4;
	if (nextEvent == nullptr) {
		btnIndex = 4;
	} else {
		int timeTillNext = int(difftime(nextEvent->unixStartTime, safeUTC.now()) / SECS_PER_MIN);
		if (timeTillNext < 15) {
			btnIndex = 0;
		} else if (timeTillNext < 30) {
			btnIndex = 1;
		} else if (timeTillNext < 60) {
			btnIndex = 2;
		} else if (timeTillNext < 90) {
			btnIndex = 3;
		} else {
			btnIndex = 4;
		}
		if (btnIndex == 2) {
			btns[BUTTON_TILLNEXT]->SetPos(button_positions[3].first, button_positions[3].second);
		} else {
			btns[BUTTON_TILLNEXT]->SetPos(button_positions[btnIndex].first,
			                              button_positions[btnIndex].second);
		}
	}
	// configure buttons to be shown
	for (int i = BUTTON_15MIN; i < BUTTON_15MIN + btnIndex; i++) {
		btns[i]->SetPos(button_positions[i - 1].first, button_positions[i - 1].second);
		btns[i]->SetHide(!isHide);
	}
	// hide buttons which cant be pressed
	if (btnIndex != 4) {
		for (int i = BUTTON_15MIN + btnIndex; i < BUTTON_CONFIRMBOOKING; i++) {
			btns[i]->SetHide(isHide);
		}
	}
	return btnIndex;
}

void hideMainButtons(bool isHide) {
	if (isHide) {
		for (int i = BUTTON_SETTINGS; i < BUTTON_TILLNEXT; i++) {
			btns[i]->SetHide(isHide);
		}
	} else {
		configureMainButtonPos(true);
		btns[BUTTON_SETTINGS]->SetHide(false);
	}
	if (nextEvent != nullptr && currentScreen == SCREEN_MAIN && currentEvent == nullptr) {
		btns[BUTTON_TILLNEXT]->SetHide(false);
	} else {
		btns[BUTTON_TILLNEXT]->SetHide(true);
	}
}

void hideBookingConfirmationButtons(bool isHide) {
	btns[BUTTON_CANCELBOOKING]->SetHide(isHide);
	btns[BUTTON_CONFIRMBOOKING]->SetHide(isHide);
}

void hideFreeConfirmationButtons(bool isHide) {
	btns[BUTTON_CANCELFREE]->SetHide(isHide);
	btns[BUTTON_CONFIRMFREE]->SetHide(isHide);
}

void hideFreeRoomButton(bool isHide) {
	btns[BUTTON_FREEROOM]->SetHide(isHide);
	if (isHide) {
		btns[BUTTON_CONTINUE]->SetHide(true);
	} else {
		if (nextEvent) {
			uint16_t deltaTime
			    = int(difftime(nextEvent->unixStartTime, currentEvent->unixEndTime) / SECS_PER_MIN);
			if (deltaTime < 15) {
				btns[BUTTON_CONTINUE]->SetHide(true);
			} else {
				btns[BUTTON_CONTINUE]->SetHide(false);
			}
		} else {
			btns[BUTTON_CONTINUE]->SetHide(isHide);
		}
	}
}

void hideConfirmBooking(uint16_t time, bool isHide) {
	lbls[LABEL_CONFIRM_BOOKING]->SetHide(isHide);
	lbls[LABEL_CONFIRM_TIME]->SetHide(isHide);
	if (!isHide) {
		lbls[LABEL_CONFIRM_TIME]->SetText(safeMyTZ.dateTime("G:i") + " - "
		                                  + safeMyTZ.dateTime(safeMyTZ.now() + time, "G:i"));
	}
}

void hideMainLabels(bool isHide) {
	lbls[LABEL_CLOCK_MID]->SetHide(isHide);
	lbls[LABEL_RESOURCE]->SetHide(isHide);
	lbls[LABEL_CURRENT_BOOKING]->SetHide(isHide);
	lbls[LABEL_BOOK_EVENT]->SetHide(isHide);
}

void hideFreeBooking(bool isHide) {
	lbls[LABEL_CONFIRM_FREE]->SetHide(isHide);
	lbls[LABEL_CONFIRM_TIME]->SetHide(isHide);
	if (!isHide) {
		lbls[LABEL_CONFIRM_TIME]->SetText(
		    safeMyTZ.dateTime(currentEvent->unixStartTime, UTC_TIME, "G:i") + " - "
		    + safeMyTZ.dateTime(currentEvent->unixEndTime, UTC_TIME, "G:i"));
	}
}

void loadNextBooking() {
	canvasNextEvent.fillCanvas(3);
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->setColors(3, 15);
	}
	lbls[LABEL_CLOCK_UP]->SetText(safeMyTZ.dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());

	for (int i = LABEL_NEXT_EVENT; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->setColors(3, 15);
	}
	lbls[LABEL_NEXT_EVENT]->SetPos(701, 161);
	lbls[LABEL_NEXT_EVENT]->SetText(l10n.msg(L10nMessage::NEXT_EVENT));
	lbls[LABEL_NEXT_EVENT_CREATOR]->SetText(nextEvent->creator);
	lbls[LABEL_NEXT_EVENT_DESC]->SetText(nextEvent->summary);
	lbls[LABEL_NEXT_EVENT_TIME]->SetText(
	    safeMyTZ.dateTime(nextEvent->unixStartTime, UTC_TIME, "G:i") + " -\n"
	    + safeMyTZ.dateTime(nextEvent->unixEndTime, UTC_TIME, "G:i"));

	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->SetHide(false);
	}
	for (int i = LABEL_NEXT_EVENT; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(false);
	}
}

void loadNextFree() {
	canvasNextEvent.fillCanvas(1);
	// set up the top bar
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->setColors(1, 15);
		lbls[i]->SetHide(false);
	}
	lbls[LABEL_CLOCK_UP]->SetText(safeMyTZ.dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());

	// hide the next event labels
	for (int i = LABEL_NEXT_EVENT_CREATOR; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(true);
	}
	lbls[LABEL_NEXT_EVENT]->SetPos(701, 359);
	lbls[LABEL_NEXT_EVENT]->SetHide(false);
	lbls[LABEL_NEXT_EVENT]->setColors(1, 15);
	lbls[LABEL_NEXT_EVENT]->SetText(l10n.msg(L10nMessage::NO_UPCOMING_EVENTS));
}

void hideCurrentBookingLabels(bool isHide) {
	lbls[LABEL_CLOCK_MID]->SetHide(isHide);
	lbls[LABEL_RESOURCE]->SetHide(isHide);
	lbls[LABEL_CURRENT_BOOKING]->SetHide(isHide);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetHide(isHide);
	lbls[LABEL_CURRENT_EVENT_DESC]->SetHide(isHide);
	lbls[LABEL_CURRENT_EVENT_TIME]->SetHide(isHide);
}

void loadCurrentBooking() {
	canvasCurrentEvent.fillCanvas(15);
	lbls[LABEL_CLOCK_MID]->setColors(15, 0);
	lbls[LABEL_RESOURCE]->setColors(15, 0);
	lbls[LABEL_CURRENT_BOOKING]->setColors(15, 0);
	lbls[LABEL_CLOCK_MID]->SetText(safeMyTZ.dateTime("G:i"));
	lbls[LABEL_RESOURCE]->SetText(resourceName);
	lbls[LABEL_CURRENT_BOOKING]->SetText(l10n.msg(L10nMessage::BOOKED));
	lbls[LABEL_BOOK_EVENT]->SetHide(true);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_DESC]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_TIME]->setColors(15, 0);

	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetText(currentEvent->creator);
	lbls[LABEL_CURRENT_EVENT_DESC]->SetText(currentEvent->summary);
	lbls[LABEL_CURRENT_EVENT_TIME]->SetText(
	    safeMyTZ.dateTime(currentEvent->unixStartTime, UTC_TIME, "G:i") + " - "
	    + safeMyTZ.dateTime(currentEvent->unixEndTime, UTC_TIME, "G:i"));

	for (int i = LABEL_CURRENT_EVENT_CREATOR; i < LABEL_CONFIRM_BOOKING; i++) {
		lbls[i]->SetHide(false);
	}
	hideMainButtons(true);
	btns[BUTTON_SETTINGS]->setPNGButton("/images/settingsBlack.png");
	btns[BUTTON_SETTINGS]->SetHide(false);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking(true);
	hideConfirmBooking(0, true);
	hideFreeRoomButton(false);
	hideCurrentBookingLabels(false);
}

void loadCurrentFree() {
	canvasCurrentEvent.fillCanvas(0);
	lbls[LABEL_RESOURCE]->setColors(0, 15);
	lbls[LABEL_RESOURCE]->SetText(resourceName);
	lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetText(l10n.msg(L10nMessage::NOT_BOOKED));
	lbls[LABEL_CLOCK_MID]->setColors(0, 15);
	lbls[LABEL_CLOCK_MID]->SetText(safeMyTZ.dateTime("G:i"));
	hideMainButtons(false);
	hideFreeRoomButton(true);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking(true);
	hideConfirmBooking(0, true);
	hideCurrentBookingLabels(true);
	hideMainLabels(false);
	lbls[LABEL_BOOK_EVENT]->SetHide(false);
	btns[BUTTON_SETTINGS]->setPNGButton("/images/settingsWhite.png");
	btns[BUTTON_SETTINGS]->SetHide(false);
}  //

void toConfirmBooking(uint16_t time, bool isTillNext) {
	tillNext = isTillNext;  // TODO: remove this
	auto res = isTillNext ? _model->calculateReserveUntilNextParams()
	                      : _model->calculateReserveParams(time * SECS_PER_MIN);
	if (res.isOk()) {
		reserveParamsPtr = res.ok();
		timeToBeBooked = difftime(reserveParamsPtr->endTime, reserveParamsPtr->startTime);
	}
	if (requireConfirmation) {
		currentScreen = SCREEN_BOOKING;
		hideMainButtons(true);
		hideMainLabels(true);
		hideNextBooking(true);
		hideBookingConfirmationButtons(false);
		hideConfirmBooking(timeToBeBooked, false);
		updateScreen(true, true, UPDATE_MODE_GLD16);
	} else {
		makeBooking(*reserveParamsPtr);
	}
}

void toFreeBooking() {
	currentScreen = SCREEN_FREEING;
	hideMainButtons(true);
	hideMainLabels(true);
	canvasCurrentEvent.fillCanvas(0);
	hideFreeRoomButton(true);
	hideNextBooking(true);
	hideCurrentBookingLabels(true);
	hideFreeConfirmationButtons(false);
	hideFreeBooking(false);
	updateScreen(true, true);
}

void makeBooking(const cal::Model::ReserveParams& params) {
	hideLoading(false);
	_model->reserveEvent(params);
}

void deleteBooking() {
	hideLoading(false);
	_model->endCurrentEvent();
}

void hideSettings(bool isHide) {
	if (!isHide) {
		String text = l10n.msg(L10nMessage::VERSION) + ": " + CURRENT_VERSION.toString() + "\n";
		if (latestVersion) {
			text += l10n.msg(L10nMessage::LATEST_VERSION) + ": " + latestVersion->toString() + "\n";
		}
		lbls[LABEL_SETTINGS_STARTUP]->SetText(text);
		lbls[LABEL_CURRENT_BOOKING]->SetPos(80, 92);
		lbls[LABEL_CURRENT_BOOKING]->SetText(l10n.msg(L10nMessage::SETTINGS));
	} else {
		lbls[LABEL_CURRENT_BOOKING]->SetPos(80, 166);
	}
	lbls[LABEL_SETTINGS_STARTUP]->SetHide(isHide);

	btns[BUTTON_SETUP]->SetHide(isHide);
	btns[BUTTON_UPDATE]->SetHide(isHide || !(latestVersion && *latestVersion != CURRENT_VERSION));
	btns[BUTTON_CANCELBOOKING]->SetHide(isHide);
	lbls[LABEL_BOOTLOG]->SetHide(true);
}

void toMainScreen(bool updateLeft, bool updateRight) {
	currentScreen = SCREEN_MAIN;
	hideSettings(true);

	if (currentEvent == nullptr) {
		loadCurrentFree();
	} else {
		loadCurrentBooking();
	}

	if (nextEvent == nullptr) {
		loadNextFree();
	} else {
		loadNextBooking();
	}
	M5.EPD.Active();
	hideLoading(true);
	updateScreen(updateLeft, updateRight);
	if (needToPutSleep) {
		M5.EPD.Sleep();
	}
}

void toSettingsScreen() {
	currentScreen = SCREEN_SETTINGS;
	canvasCurrentEvent.fillCanvas(0);
	hideMainLabels(true);
	hideMainButtons(true);
	hideNextBooking(true);
	hideCurrentBookingLabels(true);
	hideFreeRoomButton(true);
	hideSettings(false);
	if (currentEvent)
		btns[BUTTON_SETTINGS]->setPNGButton("/images/settingsWhite.png");
	btns[BUTTON_SETTINGS]->SetHide(false);
	lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetHide(false);
	updateScreen(true, true);
}

void settingsButton(epdgui_args_vector_t& args) {
	if (currentScreen == SCREEN_MAIN) {
		toSettingsScreen();
	} else if (currentScreen == SCREEN_SETTINGS) {
		gui::showBootLog();
	} else {
		toMainScreen(true, true);
	}
}

void fifteenButton(epdgui_args_vector_t& args) { toConfirmBooking(15, false); }

void thirtyButton(epdgui_args_vector_t& args) { toConfirmBooking(30, false); }

void sixtyButton(epdgui_args_vector_t& args) { toConfirmBooking(60, false); }

void ninetyButton(epdgui_args_vector_t& args) { toConfirmBooking(90, false); }

void tillNextButton(epdgui_args_vector_t& args) {
	uint16_t deltaTime = int(difftime(nextEvent->unixStartTime, safeUTC.now()) / SECS_PER_MIN);
	toConfirmBooking(deltaTime, true);
}

void confirmBookingButton(epdgui_args_vector_t& args) { makeBooking(*reserveParamsPtr); }

void cancelButton(epdgui_args_vector_t& args) { toMainScreen(true, true); }

void confirmFreeButton(epdgui_args_vector_t& args) { deleteBooking(); }

void freeRoomButton(epdgui_args_vector_t& args) { toFreeBooking(); }

void continueButton(epdgui_args_vector_t& args) {
	_model->extendCurrentEvent(15 * SECS_PER_MIN);
	_guiTask->startLoading(true);
}

void updateButton(epdgui_args_vector_t& args) {
	preferences.putBool(FORCE_UPDATE_KEY, true);
	restart();
}

void setupButton(epdgui_args_vector_t& args) { _guiTask->goSetup(false); }

void hideLoading(bool isHide) {
	lbls[LABEL_LOADING]->SetHide(isHide);
	if (!isHide) {
		hideBookingConfirmationButtons(true);
		hideConfirmBooking(0, true);
		hideFreeConfirmationButtons(true);
		hideFreeBooking(true);
		_guiTask->startLoading();
	}
}

void createButton(int ButtonEnum, String label, int16_t x, int16_t y, int16_t h, uint16_t color,
                  uint16_t txt_color, uint16_t color_pressed, bool use_bold,
                  void (*func_cb)(epdgui_args_vector_t&)) {
	int width = label.length() * PIXELS_PER_LETTER_AVG + 2 * TEXT_PADDING_X;
	btns[ButtonEnum]
	    = new EPDGUI_Button(label, x, y, width, h, color, txt_color, color_pressed, use_bold);
	EPDGUI_AddObject(btns[ButtonEnum]);
	btns[ButtonEnum]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[ButtonEnum]);
	btns[ButtonEnum]->Bind(EPDGUI_Button::EVENT_RELEASED, func_cb);
}

void createButtons() {
	btns[BUTTON_SETTINGS] = new EPDGUI_Button("", 15, 15, 64, 56, 15, 0, 0, false);
	EPDGUI_AddObject(btns[BUTTON_SETTINGS]);
	btns[BUTTON_SETTINGS]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_SETTINGS]);
	btns[BUTTON_SETTINGS]->Bind(EPDGUI_Button::EVENT_RELEASED, settingsButton);

	createButton(BUTTON_15MIN, "15", 80, 306, 77, 15, 0, 0, true, fifteenButton);
	createButton(BUTTON_30MIN, "30", 223, 306, 77, 15, 0, 0, true, thirtyButton);
	createButton(BUTTON_60MIN, "60", 371, 306, 77, 15, 0, 0, true, sixtyButton);
	createButton(BUTTON_90MIN, "90", 80, 399, 77, 15, 0, 0, true, ninetyButton);
	createButton(BUTTON_TILLNEXT, l10n.msg(L10nMessage::UNTIL_NEXT), 223, 399, 77, 15, 0, 0, true,
	             tillNextButton);
	createButton(BUTTON_CONFIRMBOOKING, l10n.msg(L10nMessage::BOOK_ROOM), 634, 399, 77, 15, 0, 0,
	             true, confirmBookingButton);
	createButton(BUTTON_CANCELBOOKING, l10n.msg(L10nMessage::CANCEL), 421, 399, 77, 0, 15, 15, true,
	             cancelButton);  // TODO: Combine cancel buttons
	createButton(BUTTON_CONFIRMFREE, l10n.msg(L10nMessage::FREE), 634, 399, 77, 15, 0, 0, true,
	             confirmFreeButton);
	createButton(BUTTON_CANCELFREE, l10n.msg(L10nMessage::CANCEL), 421, 399, 77, 0, 15, 15, true,
	             cancelButton);
	createButton(BUTTON_FREEROOM, l10n.msg(L10nMessage::FREE_ROOM), 80, 399, 77, 0, 15, 15, true,
	             freeRoomButton);
	createButton(BUTTON_CONTINUE, "+15", 410, 399, 77, 0, 15, 15, true, continueButton);
	createButton(BUTTON_UPDATE, l10n.msg(L10nMessage::UPDATE), 634, 300, 77, 15, 0, 0, true,
	             updateButton);
}

void createRegularLabels() {
	lbls[LABEL_CLOCK_UP] = new EPDGUI_Textbox(875, 16, 70, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[0]);

	lbls[LABEL_BATTERY] = new EPDGUI_Textbox(800, 16, 75, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_BATTERY]);

	lbls[LABEL_CLOCK_MID] = new EPDGUI_Textbox(80, 92, 77, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CLOCK_MID]);

	lbls[LABEL_RESOURCE] = new EPDGUI_Textbox(80, 125, 418, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_RESOURCE]);
	lbls[LABEL_RESOURCE]->AddText(resourceName);

	lbls[LABEL_NEXT_EVENT_CREATOR]
	    = new EPDGUI_Textbox(701, 246, 239, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_CREATOR]);

	lbls[LABEL_NEXT_EVENT_DESC]
	    = new EPDGUI_Textbox(701, 279, 231, 87, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_DESC]);

	lbls[LABEL_CURRENT_EVENT_CREATOR]
	    = new EPDGUI_Textbox(80, 252, 412, 40, 15, 0, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_CREATOR]);

	lbls[LABEL_CURRENT_EVENT_DESC]
	    = new EPDGUI_Textbox(80, 289, 412, 40, 15, 0, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_DESC]);

	lbls[LABEL_ERROR]
	    = new EPDGUI_Textbox(308 - 20, 0, 344 + 40, 120, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_ERROR]);
	lbls[LABEL_ERROR]->SetHide(true);

	lbls[LABEL_CONFIRM_TIME] = new EPDGUI_Textbox(144, 244, 456, 77, 0, 15, FONT_SIZE_CLOCK, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_TIME]);

	lbls[LABEL_CONFIRM_BOOKING]
	    = new EPDGUI_Textbox(144, 164, 310, 77, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_BOOKING]);
	lbls[LABEL_CONFIRM_BOOKING]->AddText(l10n.msg(L10nMessage::BOOK_ROOM_QUESTION));

	lbls[LABEL_CONFIRM_FREE]
	    = new EPDGUI_Textbox(144, 164, 450, 77, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_FREE]);
	lbls[LABEL_CONFIRM_FREE]->AddText(l10n.msg(L10nMessage::RELEASE_QUESTION));
}

void createBoldLabels() {
	lbls[LABEL_NEXT_EVENT] = new EPDGUI_Textbox(701, 161, 231, 135, 3, 15, FONT_SIZE_HEADER, true);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT]);

	lbls[LABEL_NEXT_EVENT_TIME]
	    = new EPDGUI_Textbox(701, 370, 231, 106, 3, 15, FONT_SIZE_CLOCK, true);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_TIME]);

	lbls[LABEL_BOOK_EVENT] = new EPDGUI_Textbox(80, 249, 300, 60, 0, 15, FONT_SIZE_HEADER, true);
	EPDGUI_AddObject(lbls[LABEL_BOOK_EVENT]);
	lbls[LABEL_BOOK_EVENT]->AddText(l10n.msg(L10nMessage::BOOK_ROOM));

	lbls[LABEL_CURRENT_EVENT_TIME]
	    = new EPDGUI_Textbox(80, 330, 412, 53, 15, 0, FONT_SIZE_CLOCK, true);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_TIME]);
}

void tryToPutSleep() {
	if (needToPutSleep && (currentScreen != SCREEN_BOOKING && currentScreen != SCREEN_FREEING)) {
		M5.EPD.Sleep();
	}
}

void debug(String err) {
	lbls[LABEL_ERROR]->SetHide(false);
	lbls[LABEL_ERROR]->SetText(err);
	EPDGUI_Draw(lbls[LABEL_ERROR], UPDATE_MODE_NONE);
	M5.EPD.UpdateArea(308 - 40, 0, 344 + 20, 120, UPDATE_MODE_GC16);
}

void clearDebug() {
	lbls[LABEL_ERROR]->SetText("");
	lbls[LABEL_ERROR]->SetHide(true);
	M5.EPD.UpdateArea(308 - 40, 0, 344 + 20, 120, UPDATE_MODE_GC16);
}

void toSleep() {
	if (currentScreen == SCREEN_BOOKING || currentScreen == SCREEN_FREEING) {
		toMainScreen(true, true);
	}
	M5.EPD.Sleep();
}

void setLoadingText(String text) {
	lbls[LABEL_LOADING]->SetHide(false);
	lbls[LABEL_LOADING]->SetText(text);
	M5.EPD.UpdateArea(0, 394, 960, 140, UPDATE_MODE_GC16);
}

void initLoading(bool isReverse) {
	M5.EPD.Active();
	gotResponse = false;
	_guiTask->loadNextFrame(isReverse);
}

void checkLoadNextFrame(bool isReverse) {
	if (!gotResponse) {
		_animation->showNextFrame(isReverse);
		delay(1);
		_guiTask->loadNextFrame(isReverse);
	}
}

void endLoading() {
	gotResponse = true;
	_animation->resetAnimation();
}

void shutDown(String text, bool isBootError) {
	for (std::vector<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		(*p)->SetHide(true);
	}

	M5.EPD.Active();
	canvasCurrentEvent.fillCanvas(0);
	canvasNextEvent.fillCanvas(0);
	updateScreen(true, true);
	M5.EPD.Active();
	_animation->showLogo();

	// Don't overwrite old text, because it probably contains error information
	if (lbls[LABEL_LOADING]->GetText() == "")
		_guiTask->showLoadingText(text);
}
}  // namespace

namespace gui {
void createSetupButton() {
	createButton(BUTTON_SETUP, "SETUP", 684, 399, 77, 15, 0, 0, true, setupButton);
}

void registerModel(cal::Model* model) { _model = model; }

void initGui() {
	_animation = utils::make_unique<anim::Animation>();

	M5EPD_Canvas font(&M5.EPD);
	font.setTextFont(1);
	font.loadFont(interBold, LittleFS);
	font.createRender(FONT_SIZE_BUTTON, 64);
	font.createRender(FONT_SIZE_TITLE, 128);
	font.createRender(FONT_SIZE_HEADER, 64);

	font.setTextFont(2);
	font.loadFont(interRegular, LittleFS);
	font.createRender(FONT_SIZE_NORMAL, 64);
	font.createRender(FONT_SIZE_HEADER, 64);
	font.createRender(FONT_SIZE_CLOCK, 128);
	font.createRender(FONT_SIZE_BOOTLOG, 64);
	epdgui_object_list.reserve(40);

	lbls[LABEL_LOADING] = new EPDGUI_Textbox(0, 394, 960, 140, 0, 15, FONT_SIZE_HEADER, false);
	lbls[LABEL_LOADING]->centerText();
	EPDGUI_AddObject(lbls[LABEL_LOADING]);

	lbls[LABEL_SETTINGS_STARTUP]
	    = new EPDGUI_Textbox(80, 180, 420, 360, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_SETTINGS_STARTUP]);

	lbls[LABEL_BOOTLOG] = new EPDGUI_Textbox(80, 180, 880, 360, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_BOOTLOG]);

	lbls[LABEL_CURRENT_BOOKING]
	    = new EPDGUI_Textbox(80, 164, 418, 77, 0, 15, FONT_SIZE_TITLE, true);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_BOOKING]);
	createSetupButton();
}

void displayError(gui::GUITask::GuiRequest type, const cal::Error& error) {
	_guiTask->stopLoading();
	hideLoading(true);
	debug(enumToString(type) + "\n" + error.message);
	updateScreen(true, true);
	toMainScreen(true, true);
}

void updateGui(gui::GUITask::GuiRequest type, std::shared_ptr<cal::CalendarStatus> status) {
	if (lbls[LABEL_ERROR]->GetText() != "") {
		clearDebug();
	}
	if (type == gui::GUITask::GuiRequest::RESERVE || type == gui::GUITask::GuiRequest::FREE) {
		currentScreen = SCREEN_MAIN;
	}
	_guiTask->stopLoading();
	hideLoading(true);
	updateStatus(status);
}

String enumToString(gui::GUITask::GuiRequest type) {
	switch (type) {
		case GUITask::GuiRequest::RESERVE:
			return "RESERVE";
		case GUITask::GuiRequest::FREE:
			return "FREEING";
		case GUITask::GuiRequest::MODEL:
			return "MODEL";
		case GUITask::GuiRequest::UPDATE:
			return "UPDATE";
		default:
			return "OTHER";
	}
}

void toSetupScreen(bool fromMain) {
	M5.EPD.Active();
	btns[BUTTON_SETUP]->SetHide(true);
	if (!fromMain) {
		btns[BUTTON_SETTINGS]->SetHide(true);
		btns[BUTTON_CANCELBOOKING]->SetHide(true);
	}
	lbls[LABEL_CURRENT_BOOKING]->SetHide(false);
	lbls[LABEL_SETTINGS_STARTUP]->SetHide(false);
	lbls[LABEL_LOADING]->SetHide(true);
	if (wifiManager.isAccessPoint() && fromMain) {
		M5EPD_Canvas canvasQR(&M5.EPD);
		canvasQR.createCanvas(960, 540);
		WiFiInfo info = wifiManager.getAccessPointInfo();
		const String qrString = "WIFI:S:" + info.ssid + ";T:WPA;P:" + info.password + ";;";
		canvasQR.qrcode(qrString, 510, 90, 360, 7);
		canvasQR.pushCanvas(0, 0, UPDATE_MODE_NONE);
		M5.EPD.UpdateArea(600, 140, 360, 360, UPDATE_MODE_GC16);
		lbls[LABEL_CURRENT_BOOKING]->SetPos(80, 92);
		String configString
		    = "SSID: " + info.ssid + "\nPASS: " + info.password + "\nIP: " + info.ip.toString();
		lbls[LABEL_SETTINGS_STARTUP]->SetText(configString);
	} else {
		_configStore = utils::make_unique<Config::ConfigStore>(LittleFS);
		configServer = utils::make_unique<Config::ConfigServer>(80, _configStore.get());
		wifiManager.waitWiFi(1);
		configServer->start();

		currentScreen = SCREEN_SETUP;
		String configData;
		WiFiInfo info = wifiManager.getStationInfo();
		configData = "WIFI SSID: " + info.ssid + "\nIP: " + info.ip.toString();
		lbls[LABEL_SETTINGS_STARTUP]->SetText(configData);
	}
	lbls[LABEL_CURRENT_BOOKING]->SetText("Setup");
	updateScreen(true, true);
	sleepManager.incrementTaskCounter();
	if (needToPutSleep)
		M5.EPD.Sleep();
}

void showBootLog() {
	M5.EPD.Active();
	currentScreen = SCREEN_BOOTLOG;
	for (std::vector<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		(*p)->SetHide(true);
	}
	lbls[LABEL_CURRENT_BOOKING]->SetHide(false);
	lbls[LABEL_BOOTLOG]->SetHide(false);
	btns[BUTTON_SETTINGS]->SetHide(false);
	lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetText("Bootlog");
	lbls[LABEL_BOOTLOG]->SetText("");
	canvasCurrentEvent.fillCanvas(0);
	canvasNextEvent.fillCanvas(0);
	int startTime = millis();
	std::vector<String> entries = utils::getBootLog();
	log_d("Bootlog took %s", (millis() - startTime));
	for (int i = entries.size() - 1; i >= 0; --i) {
		lbls[LABEL_BOOTLOG]->AddText(entries[i] + "\n");
		delay(2);
	}
	updateScreen(true, true);
	if (needToPutSleep)
		M5.EPD.Sleep();
}

void initMainScreen(cal::Model* model) {
	canvasCurrentEvent.createCanvas(652, 540);
	canvasNextEvent.createCanvas(308, 540);

	canvasCurrentEvent.setTextFont(1);
	createBoldLabels();
	createButtons();
	canvasCurrentEvent.setTextFont(2);
	createRegularLabels();
	registerModel(model);
}

#define GUI_QUEUE_LENGTH 40
#define GUI_TASK_PRIORITY 5
#define GUI_TASK_STACK_SIZE 4096

void task(void* arg) {
	GUITask* guiTask = static_cast<GUITask*>(arg);

	log_i("GUI Task created");

	for (;;) {
		void* reqTemp;
		xQueueReceive(guiTask->_guiQueueHandle, &reqTemp, portMAX_DELAY);
		auto counter = sleepManager.scopedTaskCount();
		auto req = toSmartPtr<GUITask::GuiQueueElement>(reqTemp);
		auto func = toSmartPtr<GUITask::QueueFunc>(req->func);
		(*func)();

		// Fix "watchdog triggered" crash by giving some processing time to idle tasks
		delay(1);
	}

	vTaskDelete(NULL);
}

void GUITask::success(GuiRequest type, std::shared_ptr<cal::CalendarStatus> status) {
	if (status) {
		enqueue(new QueueFunc([=]() { return updateGui(type, status); }));
	} else {
		enqueue(new QueueFunc([=]() { return updateGui(type, nullptr); }));
	}
}

void GUITask::error(GuiRequest type, const cal::Error& error) {
	enqueue(new QueueFunc([=]() { return displayError(type, error); }));
}

void GUITask::touchDown(const tp_finger_t& tp) {
	enqueue(new QueueFunc([=]() { return EPDGUI_Process(tp.x, tp.y); }));
}

void GUITask::touchUp() {
	enqueue(new QueueFunc([=]() { return EPDGUI_Process(); }));
}

void GUITask::sleep() {
	enqueue(new QueueFunc([=]() { return toSleep(); }));
}

void GUITask::enqueue(void* func) {
	GuiQueueElement* data = new GuiQueueElement{func};
	xQueueSend(_guiQueueHandle, (void*)&data, 0);
}

void GUITask::initMain(cal::Model* model) { initMainScreen(model); }

void GUITask::startLoading(bool isReverse) {
	enqueue(new QueueFunc([=]() { return initLoading(isReverse); }));
}

void GUITask::loadNextFrame(bool isReverse) {
	enqueue(new QueueFunc([=]() { return checkLoadNextFrame(isReverse); }));
}

void GUITask::showLoadingText(String data) {
	enqueue(new QueueFunc([=]() { return setLoadingText(data); }));
}

void GUITask::stopLoading() {
	enqueue(new QueueFunc([=]() { return endLoading(); }));
}

void GUITask::showShutdown(String shutdownText, bool isBootError) {
	enqueue(new QueueFunc([=]() { return shutDown(shutdownText, isBootError); }));
}

void GUITask::goSetup(bool fromMain) {
	enqueue(new QueueFunc([=]() { return toSetupScreen(fromMain); }));
}

GUITask::GUITask() {
	_guiTask = this;
	using namespace std::placeholders;
	M5.TP.onTouch(std::bind(&GUITask::touchDown, this, _1), std::bind(&GUITask::touchUp, this));
	_guiQueueHandle = xQueueCreate(GUI_QUEUE_LENGTH, sizeof(GUITask::GuiQueueElement*));
	xTaskCreatePinnedToCore(task, "GUI Task", GUI_TASK_STACK_SIZE, static_cast<void*>(this),
	                        GUI_TASK_PRIORITY, &_taskHandle, 0);

	gui::initGui();

	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SLEEP, [this]() { sleep(); });
	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SHUTDOWN, [this]() {
		// TODO: remember if a fatal error happened and show that instead of this shut down message
		time_t projectedTurnOnTime = sleepManager.calculateTurnOnTimeUTC(safeMyTZ.now());
		showShutdown("Shut down. Waking up at "
		             + safeMyTZ.dateTime(projectedTurnOnTime, UTC_TIME, RFC3339) + ".");
	});
}
}  // namespace gui