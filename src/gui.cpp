#include "gui.h"

#include <WiFi.h>

#include <list>
#include <map>
#include <stack>

#include "calendarApi.h"
#include "interboldttf.h"
#include "interregularttf.h"
#include "secrets.h"

enum {
	BUTTON_SETTINGS,
	BUTTON_15MIN,
	BUTTON_30MIN,
	BUTTON_60MIN,
	BUTTON_90MIN,
	BUTTON_TILLNEXT,
	BUTTON_CONFIRMBOOKING,
	BUTTON_CANCELBOOKING,
	BUTTON_FREEROOM,
	BUTTON_CONFIRMFREE,
	BUTTON_CANCELFREE,
	BUTTON_SIZE
};

enum {
	LABEL_CLOCK_UP,
	LABEL_BATTERY,
	LABEL_WIFI,
	LABEL_CLOCK_MID,
	LABEL_RESOURCE,
	LABEL_CURRENT_BOOKING,
	LABEL_BOOK_EVENT,
	LABEL_NEXT_EVENT,
	LABEL_NEXT_EVENT_CREATOR,
	LABEL_NEXT_EVENT_DESC,
	LABEL_NEXT_EVENT_TIME,
	LABEL_CURRENT_EVENT_CREATOR,
	LABEL_CURRENT_EVENT_DESC,
	LABEL_CURRENT_EVENT_TIME,
	LABEL_CONFIRM_BOOKING,
	LABEL_CONFIRM_FREE,
	LABEL_CONFIRM_TIME,
	LABEL_LOADING,
	LABEL_SIZE
};

const uint16_t FONT_SIZE_NORMAL = 24;
const uint16_t FONT_SIZE_BUTTON = 28;
const uint16_t FONT_SIZE_HEADER = 32;
const uint16_t FONT_SIZE_CLOCK = 44;
const uint16_t FONT_SIZE_TITLE = 64;

enum { SCREEN_MAIN, SCREEN_BOOKING, SCREEN_FREEING, SCREEN_SIZE };

EPDGUI_Button* btns[BUTTON_SIZE];
EPDGUI_Textbox* lbls[LABEL_SIZE];
M5EPD_Canvas canvasCurrentEvent(&M5.EPD);
M5EPD_Canvas canvasNextEvent(&M5.EPD);

std::shared_ptr<calapi::Event> currentEvent = nullptr;
std::shared_ptr<calapi::Event> nextEvent = nullptr;
Timezone* guimyTZ;

calapi::Token token = calapi::parseToken(TOKEN);
String calendarId = CALENDAR_ID;
String resourceName = "";
uint16_t timeToBeBooked = 15;

const uint32_t BAT_LOW = 3300;
const uint32_t BAT_HIGH = 4200;

uint16_t currentScreen = SCREEN_MAIN;

std::list<EPDGUI_Base*> epdgui_object_list;
uint32_t obj_id = 1;

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

void EPDGUI_Process(void) {
	for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		(*p)->UpdateState(-1, -1);
	}
}

void EPDGUI_Process(int16_t x, int16_t y) {
	for (std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin();
	     p != epdgui_object_list.end(); p++) {
		// log_d("%d, %d -> %d, %d, %d, %d", x, y, (*p)->getX(), (*p)->getY(), (*p)->getRX(),
		// (*p)->getBY());
		(*p)->UpdateState(x, y);
	}
}

void EPDGUI_Clear(void) { epdgui_object_list.clear(); }

String getBatteryPercent() {
	auto clamped = std::min(std::max(M5.getBatteryVoltage(), BAT_LOW), BAT_HIGH);
	int perc = (float)(clamped - BAT_LOW) / (float)(BAT_HIGH - BAT_LOW) * 100.0f;
	return String(perc) + "%";
}

String getWifiStatus() {
	if (WiFi.status() == WL_CONNECTED) {
		return "OK";
	} else {
		return "NOT OK";
	}
}

// TODO: dont update screen if fetched event hasnt updated
void updateStatus() {
	Serial.println("updatestatus called");
	calapi::Result<calapi::CalendarStatus> statusRes
	    = calapi::fetchCalendarStatus(token, *guimyTZ, calendarId);

	if (statusRes.isOk()) {
		auto ok = statusRes.ok();

		if (ok->currentEvent == nullptr) {
			currentEvent = nullptr;
		}

		if (ok->nextEvent == nullptr) {
			nextEvent = nullptr;
		}

		nextEvent = ok->nextEvent;
		currentEvent = ok->currentEvent;
		if (currentScreen == SCREEN_MAIN) {
			toMainScreen();
		}

	} else {
		Serial.print("Result ERROR: ");
		Serial.println(statusRes.err()->message);
	}
}

void updateScreen() {
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_NONE);
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_NONE);
	EPDGUI_Process();
	EPDGUI_Draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(UPDATE_MODE_GC16);
}

void updateClocksWifiBattery() {
	for (int i = LABEL_CLOCK_UP; i < LABEL_RESOURCE; i++) {
		lbls[i]->SetText("");
	}
	lbls[LABEL_CLOCK_UP]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_CLOCK_MID]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());
	lbls[LABEL_WIFI]->SetText(getWifiStatus());
	for (int i = LABEL_CLOCK_UP; i < LABEL_RESOURCE; i++) {
		lbls[i]->SetHide(false);
	}
	M5.EPD.UpdateArea(875, 16, 70, 40, UPDATE_MODE_GC16);
	M5.EPD.UpdateArea(780, 16, 90, 40, UPDATE_MODE_GC16);
	M5.EPD.UpdateArea(700, 16, 90, 40, UPDATE_MODE_GC16);
	M5.EPD.UpdateArea(80, 92, 77, 40, UPDATE_MODE_GC16);
}

// hides the next event on the right side
void hideNextBooking(bool isHide) {
	if (isHide) {
		canvasNextEvent.fillRect(0, 0, 308, 540, 0);
	} else {
		canvasNextEvent.fillRect(0, 0, 308, 540, 3);
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

void hideMainButtons(bool isHide) {
	for (int i = BUTTON_SETTINGS; i < BUTTON_TILLNEXT; i++) {
		btns[i]->SetHide(isHide);
	}
	if (nextEvent != nullptr && currentScreen == SCREEN_MAIN && currentEvent == nullptr) {
		btns[BUTTON_TILLNEXT]->SetEnable(true);
		btns[BUTTON_TILLNEXT]->SetHide(false);
	} else {
		btns[BUTTON_TILLNEXT]->SetEnable(false);
		btns[BUTTON_TILLNEXT]->SetHide(true);
	}
}

time_t roundToFive(time_t endTime) {
	long remainder = endTime % 300;
	return endTime - remainder + 300;
}

void hideTillNextButton() {
	btns[BUTTON_TILLNEXT]->SetEnable(true);
	btns[BUTTON_TILLNEXT]->SetHide(false);
}

void hideBookingConfirmationButtons(bool isHide) {
	btns[BUTTON_CANCELBOOKING]->SetHide(isHide);
	btns[BUTTON_CONFIRMBOOKING]->SetHide(isHide);
}

void hideFreeConfirmationButtons(bool isHide) {
	btns[BUTTON_CANCELFREE]->SetHide(isHide);
	btns[BUTTON_CONFIRMFREE]->SetHide(isHide);
}

void hideFreeRoomButton(bool isHide) { btns[BUTTON_FREEROOM]->SetHide(isHide); }

void showConfirmBooking(uint16_t time) {
	lbls[LABEL_CONFIRM_BOOKING]->SetHide(false);
	lbls[LABEL_CONFIRM_TIME]->SetHide(false);
	lbls[LABEL_CONFIRM_TIME]->SetText(guimyTZ->dateTime("G:i") + " - "
	                                  + guimyTZ->dateTime(guimyTZ->now() + time, "G:i"));
}

void hideMainLabels(bool isHide) {
	lbls[LABEL_CLOCK_MID]->SetHide(isHide);
	lbls[LABEL_RESOURCE]->SetHide(isHide);
	lbls[LABEL_CURRENT_BOOKING]->SetHide(isHide);
	lbls[LABEL_BOOK_EVENT]->SetHide(isHide);
}

void hideConfirmBooking() {
	lbls[LABEL_CONFIRM_BOOKING]->SetHide(true);
	lbls[LABEL_CONFIRM_TIME]->SetHide(true);
}

void showFreeBooking() {
	lbls[LABEL_CONFIRM_FREE]->SetHide(false);
	lbls[LABEL_CONFIRM_TIME]->SetHide(false);
	lbls[LABEL_CONFIRM_TIME]->SetText(
	    guimyTZ->dateTime(currentEvent->unixStartTime, UTC_TIME, "G:i") + " - "
	    + guimyTZ->dateTime(currentEvent->unixEndTime, UTC_TIME, "G:i"));
}

void hideFreeBooking() {
	lbls[LABEL_CONFIRM_FREE]->SetHide(true);
	lbls[LABEL_CONFIRM_TIME]->SetHide(true);
}

void loadNextBooking() {
	canvasNextEvent.fillRect(0, 0, 308, 540, 3);
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->setColors(3, 15);
	}
	lbls[LABEL_CLOCK_UP]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());
	lbls[LABEL_WIFI]->SetText(getWifiStatus());

	for (int i = LABEL_NEXT_EVENT; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->setColors(3, 15);
	}
	lbls[LABEL_NEXT_EVENT_CREATOR]->SetText(nextEvent->creator);
	lbls[LABEL_NEXT_EVENT_DESC]->SetText(nextEvent->summary);
	lbls[LABEL_NEXT_EVENT_TIME]->SetText(
	    guimyTZ->dateTime(nextEvent->unixStartTime, UTC_TIME, "G:i") + " -\n"
	    + guimyTZ->dateTime(nextEvent->unixEndTime, UTC_TIME, "G:i"));

	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->SetHide(false);
	}
	for (int i = LABEL_NEXT_EVENT; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(false);
	}
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_DU4);
	EPDGUI_Process();
	EPDGUI_Draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(UPDATE_MODE_GC16);
}

void loadNextFree() {
	canvasNextEvent.fillRect(0, 0, 308, 540, 0);
	// set up the top bar
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->setColors(0, 15);
	}
	lbls[LABEL_CLOCK_UP]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());
	lbls[LABEL_WIFI]->SetText(getWifiStatus());
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->SetHide(false);
	}

	// hide the next event labels
	for (int i = LABEL_NEXT_EVENT_CREATOR; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(true);
	}
	lbls[LABEL_NEXT_EVENT]->SetHide(false);
	// display no next booking
	lbls[LABEL_NEXT_EVENT]->setColors(0, 15);
	lbls[LABEL_NEXT_EVENT]->SetText("Ei seuraavia\nvarauksia");
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_DU);
	EPDGUI_Process();
	EPDGUI_Draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateFull(UPDATE_MODE_GC16);
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
	canvasCurrentEvent.fillRect(0, 0, 652, 540, 15);
	lbls[LABEL_CLOCK_MID]->setColors(15, 0);
	lbls[LABEL_RESOURCE]->setColors(15, 0);
	lbls[LABEL_CURRENT_BOOKING]->setColors(15, 0);
	lbls[LABEL_CLOCK_MID]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_RESOURCE]->SetText(resourceName);
	lbls[LABEL_CURRENT_BOOKING]->SetText("Varattu");
	lbls[LABEL_BOOK_EVENT]->SetHide(true);

	for (int i = LABEL_CURRENT_EVENT_CREATOR; i < LABEL_CONFIRM_BOOKING; i++) {
		lbls[i]->SetText("");
	}
	lbls[LABEL_CURRENT_EVENT_CREATOR]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_DESC]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_TIME]->setColors(15, 0);

	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetText(currentEvent->creator);
	lbls[LABEL_CURRENT_EVENT_DESC]->SetText(currentEvent->summary);
	lbls[LABEL_CURRENT_EVENT_TIME]->SetText(
	    guimyTZ->dateTime(currentEvent->unixStartTime, UTC_TIME, "G:i") + " - "
	    + guimyTZ->dateTime(currentEvent->unixEndTime, UTC_TIME, "G:i"));

	for (int i = LABEL_CURRENT_EVENT_CREATOR; i < LABEL_CONFIRM_BOOKING; i++) {
		lbls[i]->SetHide(false);
	}
	hideMainButtons(true);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking();
	hideConfirmBooking();
	hideFreeRoomButton(false);
	hideCurrentBookingLabels(false);
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_NONE);
}

void loadCurrentFree() {
	canvasCurrentEvent.fillRect(0, 0, 652, 540, 0);
	lbls[LABEL_RESOURCE]->setColors(0, 15);
	lbls[LABEL_RESOURCE]->SetText(resourceName);
	lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetText("Vapaa");
	lbls[LABEL_CLOCK_MID]->setColors(0, 15);
	lbls[LABEL_CLOCK_MID]->SetText(guimyTZ->dateTime("G:i"));
	hideMainButtons(false);
	hideFreeRoomButton(true);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking();
	hideConfirmBooking();
	hideCurrentBookingLabels(true);
	hideMainLabels(false);
	lbls[LABEL_BOOK_EVENT]->SetHide(false);
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_NONE);
}

void toConfirmBooking(uint16_t time, bool isTillNext) {
	time_t endTime = roundToFive(UTC.now() + SECS_PER_MIN * time);
	if (nextEvent != nullptr) {
		if (isTillNext || endTime > nextEvent->unixStartTime) {
			timeToBeBooked = SECS_PER_MIN * time;
		} else {
			timeToBeBooked = endTime - UTC.now();
		}
	} else {
		timeToBeBooked = endTime - UTC.now();
	}
	currentScreen = SCREEN_BOOKING;
	hideMainButtons(true);
	hideMainLabels(true);
	hideNextBooking(true);
	hideBookingConfirmationButtons(false);
	showConfirmBooking(timeToBeBooked);
	updateScreen();
}

void toFreeBooking() {
	currentScreen = SCREEN_FREEING;
	canvasCurrentEvent.fillRect(0, 0, 652, 540, 0);
	hideFreeRoomButton(true);
	hideNextBooking(true);
	hideCurrentBookingLabels(true);
	hideFreeConfirmationButtons(false);
	showFreeBooking();
	updateScreen();
}

void makeBooking(uint16_t time) {
	hideLoading(false);
	calapi::Result<calapi::Event> eventRes
	    = calapi::insertEvent(token, *guimyTZ, calendarId, UTC.now(), UTC.now() + time);

	if (eventRes.isOk()) {
		calapi::printEvent(*eventRes.ok());
		currentEvent = eventRes.ok();
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(eventRes.err()->message);
	}
	hideLoading(true);
}

void deleteBooking() {
	hideLoading(false);
	calapi::Result<calapi::Event> endedEventRes
	    = calapi::endEvent(token, *guimyTZ, calendarId, currentEvent->id);

	if (endedEventRes.isOk()) {
		calapi::printEvent(*endedEventRes.ok());
		currentEvent = nullptr;
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(endedEventRes.err()->message);
	}
	hideLoading(true);
}

void toMainScreen() {
	currentScreen = SCREEN_MAIN;

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
}
void settingsButton(epdgui_args_vector_t& args) {M5.shutdown();}

void fifteenButton(epdgui_args_vector_t& args) { toConfirmBooking(15, false); }

void thirtyButton(epdgui_args_vector_t& args) { toConfirmBooking(30, false); }

void sixtyButton(epdgui_args_vector_t& args) { toConfirmBooking(60, false); }

void ninetyButton(epdgui_args_vector_t& args) { toConfirmBooking(90, false); }

void tillNextButton(epdgui_args_vector_t& args) {
	uint16_t deltaTime = int(difftime(nextEvent->unixStartTime, UTC.now()) / SECS_PER_MIN);
	toConfirmBooking(deltaTime, true);
}

void confirmBookingButton(epdgui_args_vector_t& args) {
	makeBooking(timeToBeBooked);
	toMainScreen();
}

void cancelBookingButton(epdgui_args_vector_t& args) { toMainScreen(); }

void confirmFreeButton(epdgui_args_vector_t& args) {
	deleteBooking();
	toMainScreen();
}

void cancelFreeButton(epdgui_args_vector_t& args) { toMainScreen(); }

void freeRoomButton(epdgui_args_vector_t& args) { toFreeBooking(); }

void hideLoading(bool isHide) {
	lbls[LABEL_LOADING]->SetHide(isHide);
	EPDGUI_Process();
	EPDGUI_Draw(UPDATE_MODE_NONE);
	M5.EPD.UpdateArea(440, 240, 120, 40, UPDATE_MODE_DU);
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
	btns[BUTTON_TILLNEXT]
	    = new EPDGUI_Button("Seuraavaan tapahtumaan", 223, 399, 365, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_TILLNEXT]);
	btns[BUTTON_TILLNEXT]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_TILLNEXT]);
	btns[BUTTON_TILLNEXT]->Bind(EPDGUI_Button::EVENT_RELEASED, tillNextButton);

	// confirm booking button
	btns[BUTTON_CONFIRMBOOKING]
	    = new EPDGUI_Button("Varaa huone", 684, 399, 212, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_CONFIRMBOOKING]);
	btns[BUTTON_CONFIRMBOOKING]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0,
	                                     btns[BUTTON_CONFIRMBOOKING]);
	btns[BUTTON_CONFIRMBOOKING]->Bind(EPDGUI_Button::EVENT_RELEASED, confirmBookingButton);

	// cancel booking button
	btns[BUTTON_CANCELBOOKING] = new EPDGUI_Button("Peruuta", 521, 399, 157, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_CANCELBOOKING]);
	btns[BUTTON_CANCELBOOKING]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0,
	                                    btns[BUTTON_CANCELBOOKING]);
	btns[BUTTON_CANCELBOOKING]->Bind(EPDGUI_Button::EVENT_RELEASED, cancelBookingButton);

	// confirm booking button
	btns[BUTTON_CONFIRMFREE] = new EPDGUI_Button("Vapauta", 684, 399, 212, 77, 15, 0, 0, true);
	EPDGUI_AddObject(btns[BUTTON_CONFIRMFREE]);
	btns[BUTTON_CONFIRMFREE]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CONFIRMFREE]);
	btns[BUTTON_CONFIRMFREE]->Bind(EPDGUI_Button::EVENT_RELEASED, confirmFreeButton);

	// cancel booking button
	btns[BUTTON_CANCELFREE] = new EPDGUI_Button("Peruuta", 521, 399, 157, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_CANCELFREE]);
	btns[BUTTON_CANCELFREE]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CANCELFREE]);
	btns[BUTTON_CANCELFREE]->Bind(EPDGUI_Button::EVENT_RELEASED, cancelFreeButton);

	// free room button
	btns[BUTTON_FREEROOM] = new EPDGUI_Button("Vapauta varaus", 80, 399, 242, 77, 0, 15, 15, true);
	EPDGUI_AddObject(btns[BUTTON_FREEROOM]);
	btns[BUTTON_FREEROOM]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_FREEROOM]);
	btns[BUTTON_FREEROOM]->Bind(EPDGUI_Button::EVENT_RELEASED, freeRoomButton);
}

void createRegularLabels() {
	// upper right clock label
	lbls[LABEL_CLOCK_UP] = new EPDGUI_Textbox(875, 16, 70, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[0]);

	// battery status label
	lbls[LABEL_BATTERY] = new EPDGUI_Textbox(780, 16, 85, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_BATTERY]);

	// wifi status label
	lbls[LABEL_WIFI] = new EPDGUI_Textbox(700, 16, 80, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_WIFI]);

	// middle clock label
	lbls[LABEL_CLOCK_MID] = new EPDGUI_Textbox(80, 92, 77, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_CLOCK_MID]);

	// resource label
	lbls[LABEL_RESOURCE] = new EPDGUI_Textbox(80, 125, 418, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_RESOURCE]);
	lbls[LABEL_RESOURCE]->AddText(resourceName);

	// next event creator label
	lbls[LABEL_NEXT_EVENT_CREATOR]
	    = new EPDGUI_Textbox(701, 246, 239, 40, 3, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_CREATOR]);

	// next event desc label
	// TODO: dynamically change text formatting
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

	// current event time label
	lbls[LABEL_CURRENT_EVENT_TIME]
	    = new EPDGUI_Textbox(80, 330, 412, 53, 15, 0, FONT_SIZE_CLOCK, true);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_TIME]);

	// current event creator label
	lbls[LABEL_CONFIRM_BOOKING]
	    = new EPDGUI_Textbox(144, 164, 310, 77, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_BOOKING]);
	lbls[LABEL_CONFIRM_BOOKING]->AddText("Varataanko huone");

	// current event desc label
	lbls[LABEL_CONFIRM_FREE]
	    = new EPDGUI_Textbox(144, 164, 310, 77, 0, 15, FONT_SIZE_HEADER, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_FREE]);
	lbls[LABEL_CONFIRM_FREE]->AddText("Vapautetaanko varaus");

	lbls[LABEL_LOADING] = new EPDGUI_Textbox(440, 240, 120, 40, 0, 15, FONT_SIZE_NORMAL, false);
	EPDGUI_AddObject(lbls[LABEL_LOADING]);
	lbls[LABEL_LOADING]->AddText("Loading...");
	lbls[LABEL_LOADING]->SetHide(true);
}

void createBoldLabels() {
	// current booking status label
	lbls[LABEL_CURRENT_BOOKING]
	    = new EPDGUI_Textbox(80, 158, 418, 77, 0, 15, FONT_SIZE_TITLE, true);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_BOOKING]);

	// book event label
	lbls[LABEL_BOOK_EVENT] = new EPDGUI_Textbox(80, 241, 300, 60, 0, 15, FONT_SIZE_HEADER, true);
	EPDGUI_AddObject(lbls[LABEL_BOOK_EVENT]);
	lbls[LABEL_BOOK_EVENT]->AddText("Varaa huone");

	// next event label
	lbls[LABEL_NEXT_EVENT] = new EPDGUI_Textbox(701, 161, 231, 90, 3, 15, FONT_SIZE_HEADER, true);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT]);

	// next event time label
	lbls[LABEL_NEXT_EVENT_TIME]
	    = new EPDGUI_Textbox(701, 370, 231, 106, 3, 15, FONT_SIZE_CLOCK, true);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_TIME]);

	// current event time label
	lbls[LABEL_CONFIRM_TIME] = new EPDGUI_Textbox(144, 244, 456, 77, 0, 15, FONT_SIZE_CLOCK, true);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_TIME]);
}

void initGui(Timezone* _myTZ) {
	guimyTZ = _myTZ;
	calapi::Result<calapi::CalendarStatus> statusRes
	    = calapi::fetchCalendarStatus(token, *guimyTZ, calendarId);

	if (statusRes.isOk()) {
		auto ok = statusRes.ok();
		if (ok->currentEvent) {
			Serial.println("Result CURRENT EVENT: ");
			calapi::printEvent(*ok->currentEvent);
		}

		if (ok->nextEvent) {
			Serial.println("Result NEXT EVENT: ");
			calapi::printEvent(*ok->nextEvent);
		}
		currentEvent = ok->currentEvent;
		nextEvent = ok->nextEvent;
		resourceName = ok->name;
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(statusRes.err()->message);
	}

	canvasCurrentEvent.createCanvas(652, 540);
	canvasNextEvent.createCanvas(308, 540);

	M5EPD_Canvas boldfont(&M5.EPD);
	boldfont.loadFont(interboldttf, sizeof(interboldttf));
	boldfont.createRender(FONT_SIZE_BUTTON, 128);

	createButtons();
	boldfont.deleteCanvas();

	M5EPD_Canvas font(&M5.EPD);
	font.loadFont(interregularttf, sizeof(interregularttf));
	font.createRender(FONT_SIZE_NORMAL, 128);
	font.createRender(FONT_SIZE_HEADER, 128);
	font.createRender(FONT_SIZE_CLOCK, 128);
	font.createRender(FONT_SIZE_TITLE, 128);
	createBoldLabels();
	createRegularLabels();

	toMainScreen();
}

// Variables to store update-data from loop
uint32_t lastActiveTime = 0;
uint32_t lastFetchUpdate = 0;
uint16_t lastPosX = 0xFFFF, lastPosY = 0xFFFF;
bool isAutoUpdate = true;
const int UPDATE_INTERVAL = 60000;  // Milliseconds, how often to update status in the loop

void loopGui() {
	if (M5.TP.avaliable()) {
		M5.TP.update();
		bool is_finger_up = M5.TP.isFingerUp();
		if (is_finger_up || (lastPosX != M5.TP.readFingerX(0))
		    || (lastPosY != M5.TP.readFingerY(0))) {
			lastPosX = M5.TP.readFingerX(0);
			lastPosY = M5.TP.readFingerY(0);
			if (is_finger_up) {
				EPDGUI_Process();
				lastActiveTime = millis();
			} else {
				EPDGUI_Process(M5.TP.readFingerX(0), M5.TP.readFingerY(0));
				lastActiveTime = 0;
			}
		}

		M5.TP.flush();
	}

	if ((lastActiveTime != 0) && (millis() - lastActiveTime > 2000)) {
		if (M5.EPD.UpdateCount() > 4) {
			M5.EPD.ResetUpdateCount();
			if (isAutoUpdate) {
				M5.EPD.UpdateFull(UPDATE_MODE_GL16);
			}
		}
		lastActiveTime = 0;
	}
	if (millis() - lastFetchUpdate > UPDATE_INTERVAL) {
		lastFetchUpdate = millis();
		updateStatus();
	}
}