#include "gui/gui.h"

#include <WiFi.h>

#include <list>

#include "configServer.h"
#include "globals.h"
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

cal::Token token;
String calendarId = "";
String resourceName = "";
uint16_t timeToBeBooked = 15;

const uint32_t BAT_LOW = 3300;
const uint32_t BAT_HIGH = 4200;

uint16_t currentScreen = SCREEN_MAIN;
uint16_t currentBtnIndex = 4;

std::list<EPDGUI_Base*> epdgui_object_list;
uint32_t obj_id = 1;

bool needToPutSleep = true;
bool tillNext = false;
std::shared_ptr<cal::Model::ReserveParams> reserveParamsPtr = nullptr;

std::vector<std::pair<int, int>> button_positions
    = {{80, 306}, {223, 306}, {371, 306}, {80, 399}, {223, 399}};

void EPDGUI_AddObject(EPDGUI_Base* object) {
	obj_id++;
	epdgui_object_list.push_back(object);
}

void EPDGUI_Draw(m5epd_update_mode_t mode) {
	for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		(*p)->Draw(mode);
	}
}

void EPDGUI_Draw(EPDGUI_Base* object, m5epd_update_mode_t mode) { object->Draw(mode); }

void EPDGUI_Process(void) {
	Serial.println("Finger lifted");
	bool isGoingToSleep = false;
	for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
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
	Serial.print("Touch at coordinates ");
	Serial.print(x);
	Serial.print(" - ");
	Serial.println(y);
	sleepManager.refreshTouchWake();
	bool isGoingToSleep = false;
	for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		bool screenGoingToUpdate = (*p)->UpdateState(x, y);
		if (screenGoingToUpdate) {
			isGoingToSleep = true;
		}
	}
	needToPutSleep = !isGoingToSleep;
	tryToPutSleep();
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

void updateStatus(cal::CalendarStatus* statusCopy) {
	log_i("updatestatus called....");
	bool leftEventsEqual = false;
	bool updateRight = false;
	if (statusCopy) {
		auto status = toSmartPtr<cal::CalendarStatus>(statusCopy);
		bool leftEventsEqual = checkEventEquality(currentEvent, status->currentEvent);
		bool updateRight = checkEventEquality(nextEvent, status->nextEvent);
		resourceName = status->name;
		nextEvent = status->nextEvent;
		currentEvent = status->currentEvent;
	}
	int newBtnIndex = configureMainButtonPos(false);
	bool buttonsEqual = currentBtnIndex == newBtnIndex;
	bool updateLeft = leftEventsEqual && buttonsEqual;
	currentBtnIndex = newBtnIndex;
	if (currentScreen == SCREEN_MAIN) {
		toMainScreen(!updateLeft, !updateRight);
	}
}

void updateScreen(bool pushLeft, bool pushRight) {
	if (pushLeft) {
		canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_NONE);
	}
	if (pushRight) {
		canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_NONE);
	}
	EPDGUI_Draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(UPDATE_MODE_GC16);
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
		btns[BUTTON_TILLNEXT]->SetEnable(true);
		btns[BUTTON_TILLNEXT]->SetHide(false);
	} else {
		btns[BUTTON_TILLNEXT]->SetEnable(false);
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
	/* if(isHide) {
	    btns[BUTTON_CONTINUE]->SetHide(true);
	} else {
	    if(nextEvent) {
	        uint16_t deltaTime = int(difftime(nextEvent->unixStartTime, safeUTC.now()) /
	SECS_PER_MIN); if (deltaTime <= 15) { btns[BUTTON_CONTINUE]->SetHide(true); } else {
	            btns[BUTTON_CONTINUE]->SetHide(false);
	        }
	    }
	} */
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
	lbls[LABEL_NEXT_EVENT]->SetText("Seuraava\nvaraus");
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
	canvasNextEvent.fillCanvas(0);
	// set up the top bar
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->setColors(0, 15);
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
	lbls[LABEL_NEXT_EVENT]->setColors(0, 15);
	lbls[LABEL_NEXT_EVENT]->SetText("Ei seuraavia\nvarauksia");
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
	lbls[LABEL_CURRENT_BOOKING]->SetText("Varattu");
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
	lbls[LABEL_CURRENT_BOOKING]->SetText("Vapaa");
	lbls[LABEL_CLOCK_MID]->setColors(0, 15);
	lbls[LABEL_CLOCK_MID]->SetText(safeMyTZ.dateTime("G:i"));
	hideMainButtons(false);  //
	hideFreeRoomButton(true);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking(true);
	hideConfirmBooking(0, true);
	hideCurrentBookingLabels(true);
	hideMainLabels(false);
	lbls[LABEL_BOOK_EVENT]->SetHide(false);
	btns[BUTTON_SETTINGS]->SetHide(false);
}

void toConfirmBooking(uint16_t time, bool isTillNext) {
	tillNext = isTillNext;  // TODO: remove this
	auto res = isTillNext ? _model->calculateReserveUntilNextParams()
	                      : _model->calculateReserveParams(time * SECS_PER_MIN);
	if (res.isOk()) {
		reserveParamsPtr = res.ok();
	}
	timeToBeBooked = difftime(reserveParamsPtr->endTime, reserveParamsPtr->startTime);
	Serial.print("Time to be booked is ");
	Serial.println(timeToBeBooked);
	currentScreen = SCREEN_BOOKING;
	hideMainButtons(true);
	hideMainLabels(true);
	hideNextBooking(true);
	hideBookingConfirmationButtons(false);
	hideConfirmBooking(timeToBeBooked, false);
	updateScreen(true, true);
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
		lbls[LABEL_SETTINGS_STARTUP]->SetGeometry(80, 158, 500, 150);
		lbls[LABEL_SETTINGS_STARTUP]->SetText("Viime käynnistys:\n" + safeMyTZ.dateTime(RFC3339));
		lbls[LABEL_CURRENT_BOOKING]->SetPos(80, 92);
		lbls[LABEL_CURRENT_BOOKING]->SetText("Asetukset");
	} else {
		lbls[LABEL_CURRENT_BOOKING]->SetPos(80, 158);
	}
	lbls[LABEL_SETTINGS_STARTUP]->SetHide(isHide);

	btns[BUTTON_SETUP]->SetHide(isHide);
	btns[BUTTON_CANCELBOOKING]->SetHide(isHide);
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
	//M5.EPD.Active();
	hideLoading(true);
	updateScreen(updateLeft, updateRight);
	//M5.EPD.Sleep();
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

void confirmBookingButton(epdgui_args_vector_t& args) {
	makeBooking(*reserveParamsPtr);
	toMainScreen(true, true);
}

void cancelButton(epdgui_args_vector_t& args) { toMainScreen(true, true); }

void confirmFreeButton(epdgui_args_vector_t& args) {
	deleteBooking();
	toMainScreen(true, true);
}

void freeRoomButton(epdgui_args_vector_t& args) { toFreeBooking(); }

void continueButton(epdgui_args_vector_t& args) {}

void setupButton(epdgui_args_vector_t& args) { gui::toSetupScreen(); }

void hideLoading(bool isHide) {
	if (isHide) {
		if (currentEvent && currentScreen == SCREEN_MAIN) {
			lbls[LABEL_LOADING]->setColors(15, 15);
		} else {
			lbls[LABEL_LOADING]->setColors(0, 0);
		}
		EPDGUI_Draw(lbls[LABEL_LOADING], UPDATE_MODE_NONE);
		M5.EPD.UpdateArea(440, 240, 120, 40, UPDATE_MODE_DU4);
		lbls[LABEL_LOADING]->SetHide(isHide);
	} else {
		lbls[LABEL_LOADING]->SetHide(isHide);
		lbls[LABEL_LOADING]->setColors(0, 15);
		EPDGUI_Draw(lbls[LABEL_LOADING], UPDATE_MODE_NONE);
		M5.EPD.UpdateArea(440, 240, 120, 40, UPDATE_MODE_DU4);
	}
}

void createButtons() {
	// Options button
	btns[BUTTON_SETTINGS] = new EPDGUI_Button("o", 0, 0, 64, 64, 0, 15, 15, false);
	EPDGUI_AddObject(btns[BUTTON_SETTINGS]);
	btns[BUTTON_SETTINGS]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_SETTINGS]);
	btns[BUTTON_SETTINGS]->Bind(EPDGUI_Button::EVENT_RELEASED, settingsButton);

	// 15 min booking button
	btns[BUTTON_15MIN] = new EPDGUI_Button("15", 80, 306, 135, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_15MIN]);
	btns[BUTTON_15MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_15MIN]);
	btns[BUTTON_15MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, fifteenButton);

	// 30 min booking button
	btns[BUTTON_30MIN] = new EPDGUI_Button("30", 223, 306, 135, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_30MIN]);
	btns[BUTTON_30MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_30MIN]);
	btns[BUTTON_30MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, thirtyButton);

	// 60 min booking button
	btns[BUTTON_60MIN] = new EPDGUI_Button("60", 371, 306, 135, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_60MIN]);
	btns[BUTTON_60MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_60MIN]);
	btns[BUTTON_60MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, sixtyButton);

	// 90 min booking button
	btns[BUTTON_90MIN] = new EPDGUI_Button("90", 80, 399, 135, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_90MIN]);
	btns[BUTTON_90MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_90MIN]);
	btns[BUTTON_90MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, ninetyButton);

	// book till next event button
	btns[BUTTON_TILLNEXT] = new EPDGUI_Button("SEURAAVAAN ASTI", 223, 399, 365, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_TILLNEXT]);
	btns[BUTTON_TILLNEXT]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_TILLNEXT]);
	btns[BUTTON_TILLNEXT]->Bind(EPDGUI_Button::EVENT_RELEASED, tillNextButton);

	// confirm booking button
	btns[BUTTON_CONFIRMBOOKING]
	    = new EPDGUI_Button("VARAA HUONE", 684, 399, 232, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_CONFIRMBOOKING]);
	btns[BUTTON_CONFIRMBOOKING]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0,
	                                     btns[BUTTON_CONFIRMBOOKING]);
	btns[BUTTON_CONFIRMBOOKING]->Bind(EPDGUI_Button::EVENT_RELEASED, confirmBookingButton);

	// cancel booking button
	btns[BUTTON_CANCELBOOKING] = new EPDGUI_Button("PERUUTA", 521, 399, 157, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_CANCELBOOKING]);
	btns[BUTTON_CANCELBOOKING]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0,
	                                    btns[BUTTON_CANCELBOOKING]);
	btns[BUTTON_CANCELBOOKING]->Bind(EPDGUI_Button::EVENT_RELEASED, cancelButton);

	// confirm booking button
	btns[BUTTON_CONFIRMFREE] = new EPDGUI_Button("VAPAUTA", 684, 399, 167, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_CONFIRMFREE]);
	btns[BUTTON_CONFIRMFREE]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CONFIRMFREE]);
	btns[BUTTON_CONFIRMFREE]->Bind(EPDGUI_Button::EVENT_RELEASED, confirmFreeButton);

	// cancel booking button
	btns[BUTTON_CANCELFREE] = new EPDGUI_Button("PERUUTA", 521, 399, 157, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_CANCELFREE]);
	btns[BUTTON_CANCELFREE]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CANCELFREE]);
	btns[BUTTON_CANCELFREE]->Bind(EPDGUI_Button::EVENT_RELEASED, cancelButton);

	// free room button
	btns[BUTTON_FREEROOM] = new EPDGUI_Button("VAPAUTA VARAUS", 80, 399, 287, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_FREEROOM]);
	btns[BUTTON_FREEROOM]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_FREEROOM]);
	btns[BUTTON_FREEROOM]->Bind(EPDGUI_Button::EVENT_RELEASED, freeRoomButton);

	// continue current booking
	btns[BUTTON_CONTINUE] = new EPDGUI_Button("+15", 330, 399, 135, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_CONTINUE]);
	btns[BUTTON_CONTINUE]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CONTINUE]);
	btns[BUTTON_CONTINUE]->Bind(EPDGUI_Button::EVENT_RELEASED, continueButton);
	btns[BUTTON_CONTINUE]->SetHide(true);

	// setup mode
	btns[BUTTON_SETUP] = new EPDGUI_Button("SETUP", 684, 399, 157, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_SETUP]);
	btns[BUTTON_SETUP]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_SETUP]);
	btns[BUTTON_SETUP]->Bind(EPDGUI_Button::EVENT_RELEASED, setupButton);
}

void createRegularLabels() {
	// upper right clock label
	lbls[LABEL_CLOCK_UP] = new EPDGUI_Textbox(875, 16, 70, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[0]);

	// battery status label
	lbls[LABEL_BATTERY] = new EPDGUI_Textbox(800, 16, 75, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_BATTERY]);

	// middle clock label
	lbls[LABEL_CLOCK_MID] = new EPDGUI_Textbox(80, 92, 77, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CLOCK_MID]);

	// resource label
	lbls[LABEL_RESOURCE] = new EPDGUI_Textbox(80, 125, 418, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_RESOURCE]);
	lbls[LABEL_RESOURCE]->AddText(resourceName);

	// book event label
	lbls[LABEL_BOOK_EVENT] = new EPDGUI_Textbox(80, 241, 300, 60, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_BOOK_EVENT]);
	lbls[LABEL_BOOK_EVENT]->AddText("Varaa huone");

	// next event label
	lbls[LABEL_NEXT_EVENT] = new EPDGUI_Textbox(701, 161, 231, 90, 3, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT]);

	// next event creator label
	lbls[LABEL_NEXT_EVENT_CREATOR]
	    = new EPDGUI_Textbox(701, 246, 239, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_CREATOR]);

	// next event desc label
	lbls[LABEL_NEXT_EVENT_DESC]
	    = new EPDGUI_Textbox(701, 279, 231, 87, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_DESC]);

	// current event creator label
	lbls[LABEL_CURRENT_EVENT_CREATOR]
	    = new EPDGUI_Textbox(80, 264, 412, 40, 15, 0, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_CREATOR]);

	// current event desc label
	lbls[LABEL_CURRENT_EVENT_DESC]
	    = new EPDGUI_Textbox(80, 297, 412, 40, 15, 0, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_DESC]);

	// current event creator label
	lbls[LABEL_CONFIRM_BOOKING]
	    = new EPDGUI_Textbox(144, 164, 310, 77, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_BOOKING]);
	lbls[LABEL_CONFIRM_BOOKING]->AddText("Varataanko huone");

	// current event desc label
	lbls[LABEL_CONFIRM_FREE]
	    = new EPDGUI_Textbox(144, 164, 450, 77, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_FREE]);
	lbls[LABEL_CONFIRM_FREE]->AddText("Vapautetaanko varaus");

	lbls[LABEL_LOADING] = new EPDGUI_Textbox(440, 240, 120, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_LOADING]);
	lbls[LABEL_LOADING]->AddText("Loading...");
	lbls[LABEL_LOADING]->SetHide(true);

	lbls[LABEL_ERROR] = new EPDGUI_Textbox(308, 0, 344, 120, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_ERROR]);
	lbls[LABEL_ERROR]->SetHide(true);

	lbls[LABEL_SETTINGS_STARTUP]
	    = new EPDGUI_Textbox(80, 158, 500, 150, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_SETTINGS_STARTUP]);
	lbls[LABEL_SETTINGS_STARTUP]->SetHide(true);

	// next event time label
	lbls[LABEL_NEXT_EVENT_TIME]
	    = new EPDGUI_Textbox(701, 370, 231, 106, 3, 15, FONT_SIZE_CLOCK, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_TIME]);

	// current event time label
	lbls[LABEL_CONFIRM_TIME] = new EPDGUI_Textbox(144, 244, 456, 77, 0, 15, FONT_SIZE_CLOCK, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_TIME]);

	// current event time label
	lbls[LABEL_CURRENT_EVENT_TIME]
	    = new EPDGUI_Textbox(80, 330, 412, 53, 15, 0, FONT_SIZE_CLOCK, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_TIME]);
}

void createBoldLabels() {
	// current booking status label
	lbls[LABEL_CURRENT_BOOKING]
	    = new EPDGUI_Textbox(80, 160, 418, 77, 0, 15, FONT_SIZE_TITLE, true);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_BOOKING]);
}

void tryToPutSleep() {
	/* if (needToPutSleep && (currentScreen != SCREEN_BOOKING && currentScreen != SCREEN_FREEING)) {
		M5.EPD.Sleep();
	} */
}
void debug(String err) {
	lbls[LABEL_ERROR]->SetHide(false);
	lbls[LABEL_ERROR]->SetText(err);
	M5.EPD.UpdateArea(308, 0, 344, 120, UPDATE_MODE_GC16);
}

void clearDebug() {
	lbls[LABEL_ERROR]->SetText("");
	lbls[LABEL_ERROR]->SetHide(true);
	M5.EPD.UpdateArea(308, 0, 344, 120, UPDATE_MODE_GC16);
}
}  // namespace

namespace gui {

void registerModel(cal::Model* model) { _model = model; }

void initGui(Config::ConfigStore* configStore) {
	JsonObjectConst config = configStore->getConfigJson();
	bool loadSetup = config.begin() == config.end();
	canvasCurrentEvent.createCanvas(652, 540);
	canvasNextEvent.createCanvas(308, 540);

	M5EPD_Canvas boldfont(&M5.EPD);
	boldfont.setTextFont(1);
	boldfont.loadFont(interBold, LittleFS);
	boldfont.createRender(FONT_SIZE_BUTTON, 64);
	boldfont.createRender(FONT_SIZE_TITLE, 128);
	boldfont.createRender(FONT_SIZE_HEADER, 64);
	createBoldLabels();
	createButtons();

	M5EPD_Canvas font(&M5.EPD);
	font.setTextFont(2);
	font.loadFont(interRegular, LittleFS);
	font.createRender(FONT_SIZE_NORMAL, 64);
	font.createRender(FONT_SIZE_HEADER, 64);
	font.createRender(FONT_SIZE_CLOCK, 128);
	createRegularLabels();

	M5.EPD.Active();
	if (loadSetup) {
		log_i("Going to setupscreen");
		for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
		     p != epdgui_object_list.end(); p++) {
			(*p)->SetHide(true);
		}
		hideSettings(false);
		lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
		lbls[LABEL_CURRENT_BOOKING]->SetHide(false);
		toSetupScreen();
	}
}

void displayError(gui::GUITask::GuiRequest type, const cal::Error& error) {
	hideLoading(true);
	debug("Code " + enumToString(type) + " " + String(error.code) + "\n" + "- "
	      + String(error.message));
}
void updateGui(gui::GUITask::GuiRequest type, cal::CalendarStatus* status) {
	hideLoading(true);
	updateStatus(status);
	log_i("updateGui was called");
}

String enumToString(gui::GUITask::GuiRequest type) {  // TODO: this can be done with an array
	switch (type) {
		case GUITask::GuiRequest::RESERVE:
			return "RESERVE";
			break;
		case GUITask::GuiRequest::FREE:
			return "FREEING";
			break;
		case GUITask::GuiRequest::MODEL:
			return "MODEL";
			break;
		case GUITask::GuiRequest::UPDATE:
			return "UPDATE";
			break;
		default:
			return "OTHER";
			break;
	}
}

void toSetupScreen() {
	currentScreen = SCREEN_SETUP;
	canvasCurrentEvent.fillCanvas(0);
	btns[BUTTON_SETUP]->SetHide(true);
	btns[BUTTON_CANCELBOOKING]->SetHide(true);
	lbls[LABEL_CURRENT_BOOKING]->SetHide(false);
	lbls[LABEL_SETTINGS_STARTUP]->SetHide(false);
	lbls[LABEL_CURRENT_BOOKING]->SetText("Setup");
	String configData = "";

	if (utils::isAP) {
		configData += "Yhdistä WiFiin:\nSSID: ";
		configData += utils::getApSSID() + "\n";
		configData += "Salasana: ";
		configData += utils::getApPassword() + "\n";
		configData += "Navigoi osoitteeseen: 192.168.69.1";
		const String qrString
		    = "WIFI:S:" + utils::getApSSID() + ";T:WPA;P:" + utils::getApPassword() + ";;";
		canvasNextEvent.qrcode(qrString, 0, 120, 300, 7);
	} else {
		utils::ensureWiFi();
		String wifiSSID = WiFi.SSID();
		configData += "Yhdistetty WiFiin: ";
		configData += wifiSSID + "\nLaitteen IP: ";
		configData += WiFi.localIP().toString();
	}

	if (!utils::isSetupMode()) {
		utils::setupMode();
	}
	lbls[LABEL_SETTINGS_STARTUP]->SetText(configData);
	updateScreen(true, true);
}

void showBootLog() {
	currentScreen = SCREEN_BOOTLOG;
	for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		(*p)->SetHide(true);
	}
	lbls[LABEL_CURRENT_BOOKING]->SetHide(false);
	lbls[LABEL_SETTINGS_STARTUP]->SetHide(false);
	btns[BUTTON_SETTINGS]->SetHide(false);
	lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetPos(40, 92);
	lbls[LABEL_CURRENT_BOOKING]->SetText("Bootlog");
	lbls[LABEL_SETTINGS_STARTUP]->SetGeometry(40, 180, 500, 460);
	lbls[LABEL_SETTINGS_STARTUP]->SetText("");
	canvasCurrentEvent.fillCanvas(0);
	canvasNextEvent.fillCanvas(0);
	std::vector<String> entries = utils::getBootLog();
	for (int i = entries.size() - 1; i >= 0; --i) {
		lbls[LABEL_SETTINGS_STARTUP]->AddText(entries[i] + "\n");
	}

	updateScreen(true, true);
}

#define GUI_QUEUE_LENGTH 15
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
	}

	vTaskDelete(NULL);
}

// TODO: use type -parameter
void GUITask::success(GuiRequest type, cal::CalendarStatus* status) {
	cal::CalendarStatus* statusCopy = new cal::CalendarStatus(*status);
	enqueue(ActionType::SUCCESS, new QueueFunc([=]() { return updateGui(type, statusCopy); }));
}

// TODO: use type -parameter
void GUITask::error(GuiRequest type, const cal::Error& error) {
	enqueue(ActionType::ERROR, new QueueFunc([=]() { return displayError(type, error); }));
}

void GUITask::touchDown(const tp_finger_t& tp) {
	enqueue(ActionType::TOUCH_DOWN, new QueueFunc([=]() { return EPDGUI_Process(tp.x, tp.y); }));
}

void GUITask::touchUp() {
	enqueue(ActionType::TOUCH_UP, new QueueFunc([=]() { return EPDGUI_Process(); }));
}

void GUITask::enqueue(ActionType at, void* func) {
	GuiQueueElement* data = new GuiQueueElement{at, func};
	xQueueSend(_guiQueueHandle, (void*)&data, 0);
};

GUITask::GUITask(Config::ConfigStore* configStore, cal::Model* model) {
	using namespace std::placeholders;
	M5.TP.onTouch(std::bind(&GUITask::touchDown, this, _1), std::bind(&GUITask::touchUp, this));
	BaseType_t xReturned;
	_guiQueueHandle = xQueueCreate(GUI_QUEUE_LENGTH, sizeof(GUITask::GuiQueueElement*));
	xReturned = xTaskCreatePinnedToCore(task,
										"GUI Task",
										GUI_TASK_STACK_SIZE,
										static_cast<void*>(this),
	                              		GUI_TASK_PRIORITY,
										&_taskHandle, 1);
	Serial.print("xReturned value is ");
	Serial.println(xReturned);
	gui::registerModel(model);

	gui::initGui(configStore);
	if (_guiQueueHandle != NULL) {
		log_i("GUI Task: queue created");
	} else {
		log_i("GUI Task: queue create failed");
	}
}
}  // namespace gui