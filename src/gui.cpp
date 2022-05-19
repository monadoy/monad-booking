#include "gui.h"

#include <WiFi.h>

#include "calendarApi.h"
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
	LABEL_SIZE
};

enum 
{
	SCREEN_MAIN,
	SCREEN_BOOKING,
	SCREEN_FREEING,
	SCREEN_SIZE
};

#define TIME_FONT_SIZE = 48;
#define DEFAULT_FONT_SIZE = 24;
#define MAIN_HEADER_FONT_SIZE = 60;
#define MID_HEADER_FONT_SIZE = 32;

EPDGUI_Button* btns[BUTTON_SIZE];
EPDGUI_Textbox* lbls[LABEL_SIZE];
M5EPD_Canvas canvasCurrentEvent(&M5.EPD);
M5EPD_Canvas canvasNextEvent(&M5.EPD);

std::shared_ptr<calapi::Event> currentEvent;
std::shared_ptr<calapi::Event> nextEvent;
Timezone* guimyTZ;

calapi::Token token = calapi::parseToken(TOKEN);
String calendarId = CALENDAR_ID;
String resourceName = "";
uint16_t timeToBeBooked = 15;

const uint32_t BAT_LOW = 3300;
const uint32_t BAT_HIGH = 4350;
const int UPDATE_INTERVAL = 30;

uint16_t currentScreen = SCREEN_MAIN;

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
	Serial.println(F("call updatestatus"));
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
	if(currentScreen == SCREEN_MAIN) {
		toMainScreen();
	}
	guimyTZ->setEvent(updateStatus, guimyTZ->now()+UPDATE_INTERVAL);
	Serial.println("set next call event");
}

// ei tarvii
void updateScreen() {
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_DU4);
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_DU4);
	EPDGUI_Run();
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
	for (int i = BUTTON_SETTINGS; i < BUTTON_CONFIRMBOOKING; i++) {
		btns[i]->SetHide(isHide);
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

void hideFreeRoomButton(bool isHide) { btns[BUTTON_FREEROOM]->SetHide(isHide); }

void showConfirmBooking(uint16_t time) {
	lbls[LABEL_CONFIRM_BOOKING]->SetHide(false);
	lbls[LABEL_CONFIRM_TIME]->SetHide(false);
	lbls[LABEL_CONFIRM_TIME]->SetText(
	    guimyTZ->dateTime("G:i") + " - "
	    + guimyTZ->dateTime(guimyTZ->now() + SECS_PER_MIN * time, "G:i"));
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
		lbls[i]->SetHide(false);
		lbls[i]->setColors(3, 15);
		lbls[i]->SetTextSize(24);
		lbls[i]->SetText("");
	}
	lbls[LABEL_CLOCK_UP]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());
	lbls[LABEL_WIFI]->SetText(getWifiStatus());

	for (int i = LABEL_NEXT_EVENT; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(false);
		lbls[i]->setColors(3, 15);
		lbls[i]->SetTextSize(24);
		lbls[i]->SetText("");
	}
	lbls[LABEL_NEXT_EVENT_CREATOR]->SetText(nextEvent->creator);
	lbls[LABEL_NEXT_EVENT_DESC]->SetText(nextEvent->summary);
	lbls[LABEL_NEXT_EVENT_TIME]->SetText(
	    guimyTZ->dateTime(nextEvent->unixStartTime, UTC_TIME, "G:i") + " -\n"
	    + guimyTZ->dateTime(nextEvent->unixEndTime, UTC_TIME, "G:i"));
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_DU4);
	EPDGUI_Run();
}

void loadNextFree() {
	canvasNextEvent.fillRect(0, 0, 308, 540, 0);
	// set up the top bar
	for (int i = LABEL_CLOCK_UP; i < LABEL_CLOCK_MID; i++) {
		lbls[i]->setColors(0, 15);
		lbls[i]->SetHide(false);
		lbls[i]->SetTextSize(24);
		lbls[i]->SetText("");
	}
	lbls[LABEL_CLOCK_UP]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_BATTERY]->SetText(getBatteryPercent());
	lbls[LABEL_WIFI]->SetText(getWifiStatus());

	// hide the next event labels
	for (int i = LABEL_NEXT_EVENT_CREATOR; i < LABEL_CURRENT_EVENT_CREATOR; i++) {
		lbls[i]->SetHide(true);
	}
	lbls[LABEL_NEXT_EVENT]->SetHide(false);
	// display no next booking
	lbls[LABEL_NEXT_EVENT]->setColors(0, 15);
	lbls[LABEL_BOOK_EVENT]->SetText("");
	lbls[LABEL_NEXT_EVENT]->SetText("Ei seuraavia\nvarauksia");
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_DU4);
	EPDGUI_Run();
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
	hideMainButtons(true);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking();
	hideConfirmBooking();
	hideFreeRoomButton(false);
	hideCurrentBookingLabels(false);
	lbls[LABEL_BOOK_EVENT]->SetHide(true);

	for (int i = LABEL_CURRENT_EVENT_CREATOR; i < LABEL_CONFIRM_BOOKING; i++) {
		lbls[i]->SetHide(false);
		lbls[i]->SetText("");
	}

	lbls[LABEL_CLOCK_MID]->setColors(15, 0);
	lbls[LABEL_RESOURCE]->setColors(15, 0);
	lbls[LABEL_CURRENT_BOOKING]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_DESC]->setColors(15, 0);
	lbls[LABEL_CURRENT_EVENT_TIME]->setColors(15, 0);

	lbls[LABEL_CLOCK_MID]->SetTextSize(24);
	lbls[LABEL_RESOURCE]->SetTextSize(24);
	lbls[LABEL_CURRENT_BOOKING]->SetTextSize(24);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetTextSize(24);
	lbls[LABEL_CURRENT_EVENT_DESC]->SetTextSize(24);
	lbls[LABEL_CURRENT_EVENT_TIME]->SetTextSize(24);

	lbls[LABEL_CLOCK_MID]->SetText("");
	lbls[LABEL_RESOURCE]->SetText("");
	lbls[LABEL_CURRENT_BOOKING]->SetText("");
	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetText("");
	lbls[LABEL_CURRENT_EVENT_DESC]->SetText("");
	lbls[LABEL_CURRENT_EVENT_TIME]->SetText("");

	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetText(currentEvent->creator);
	lbls[LABEL_CURRENT_EVENT_DESC]->SetText(currentEvent->summary);
	lbls[LABEL_CURRENT_EVENT_TIME]->SetText(
	    guimyTZ->dateTime(currentEvent->unixStartTime, UTC_TIME, "G:i") + " - "
	    + guimyTZ->dateTime(currentEvent->unixEndTime, UTC_TIME, "G:i"));
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void loadCurrentFree() {
	canvasCurrentEvent.fillRect(0, 0, 652, 540, 0);
	hideMainButtons(false);
	hideFreeRoomButton(true);
	hideBookingConfirmationButtons(true);
	hideFreeConfirmationButtons(true);
	hideFreeBooking();
	hideConfirmBooking();
	hideCurrentBookingLabels(true);
	hideMainLabels(false);
	lbls[LABEL_RESOURCE]->setColors(0, 15);
	lbls[LABEL_RESOURCE]->SetText("");
	lbls[LABEL_RESOURCE]->SetText("Pieni neukkari 2");
	lbls[LABEL_CURRENT_BOOKING]->setColors(0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetText("");
	lbls[LABEL_CURRENT_BOOKING]->SetText("Vapaa");
	lbls[LABEL_CLOCK_MID]->setColors(0, 15);
	lbls[LABEL_CLOCK_MID]->SetText("");
	lbls[LABEL_CLOCK_MID]->SetText(guimyTZ->dateTime("G:i"));
	lbls[LABEL_BOOK_EVENT]->setColors(0, 15);
	lbls[LABEL_BOOK_EVENT]->SetText("");
	lbls[LABEL_BOOK_EVENT]->SetText("Varaa huone");
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void toConfirmBooking(uint16_t time) {
	currentScreen = SCREEN_BOOKING;
	hideMainButtons(true);
	hideMainLabels(true);
	hideNextBooking(true);
	hideBookingConfirmationButtons(false);
	timeToBeBooked = time;
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
	calapi::Result<calapi::Event> eventRes = calapi::insertEvent(
	    token, *guimyTZ, calendarId, UTC.now(), UTC.now() + SECS_PER_MIN * time);

	if (eventRes.isOk()) {
		calapi::printEvent(*eventRes.ok());
		currentEvent = eventRes.ok();
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(eventRes.err()->message);
	}
}

void deleteBooking() {
	calapi::Result<calapi::Event> endedEventRes
	    = calapi::endEvent(token, *guimyTZ, calendarId, currentEvent->id);

	if (endedEventRes.isOk()) {
		calapi::printEvent(*endedEventRes.ok());
		currentEvent = nullptr;
	} else {
		Serial.print("Result ERROR: ");
		Serial.println(endedEventRes.err()->message);
	}
}

void toMainScreen() {
	// check for current event
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
	updateScreen();
}
void settingsButton(epdgui_args_vector_t& args) {}

void fifteenButton(epdgui_args_vector_t& args) { toConfirmBooking(15); }

void thirtyButton(epdgui_args_vector_t& args) { toConfirmBooking(30); }

void sixtyButton(epdgui_args_vector_t& args) { toConfirmBooking(60); }

void ninetyButton(epdgui_args_vector_t& args) { toConfirmBooking(90); }

void tillNextButton(epdgui_args_vector_t& args) { toConfirmBooking(120); }

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

void createLabels() {
	// upper right clock label
	lbls[LABEL_CLOCK_UP] = new EPDGUI_Textbox(875, 16, 70, 40, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[0]);
	lbls[LABEL_CLOCK_UP]->AddText("13");

	// battery status label
	lbls[LABEL_BATTERY] = new EPDGUI_Textbox(816, 16, 60, 40, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_BATTERY]);
	lbls[LABEL_BATTERY]->AddText("44% ");

	// wifi status label
	lbls[LABEL_WIFI] = new EPDGUI_Textbox(752, 16, 51, 40, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_WIFI]);
	lbls[LABEL_WIFI]->AddText("OK");

	// middle clock label
	lbls[LABEL_CLOCK_MID] = new EPDGUI_Textbox(80, 92, 77, 40, 0, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CLOCK_MID]);
	lbls[LABEL_CLOCK_MID]->AddText("13:39");
	// 498
	// resource label
	lbls[LABEL_RESOURCE] = new EPDGUI_Textbox(80, 125, 418, 40, 0, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_RESOURCE]);
	lbls[LABEL_RESOURCE]->AddText("Pieni neukkari 2");

	// current booking status label
	lbls[LABEL_CURRENT_BOOKING] = new EPDGUI_Textbox(80, 158, 418, 77, 0, 15, 24, false);
	lbls[LABEL_CURRENT_BOOKING]->AddText("Vapaa");
	EPDGUI_AddObject(lbls[LABEL_CURRENT_BOOKING]);

	// book event label
	lbls[LABEL_BOOK_EVENT] = new EPDGUI_Textbox(80, 241, 240, 60, 0, 15, 24, false);
	lbls[LABEL_BOOK_EVENT]->AddText("Varaa huone");
	EPDGUI_AddObject(lbls[LABEL_BOOK_EVENT]);

	// next event label
	lbls[LABEL_NEXT_EVENT] = new EPDGUI_Textbox(701, 161, 231, 78, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT]);
	lbls[LABEL_NEXT_EVENT]->AddText("Seuraava\nvaraus");

	// next event creator label
	lbls[LABEL_NEXT_EVENT_CREATOR] = new EPDGUI_Textbox(701, 246, 239, 40, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_CREATOR]);
	lbls[LABEL_NEXT_EVENT_CREATOR]->AddText("Kalle Kallenen 2");

	// next event desc label
	// here height is n*27 where n is the number of rows
	lbls[LABEL_NEXT_EVENT_DESC] = new EPDGUI_Textbox(701, 279, 231, 87, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_DESC]);
	lbls[LABEL_NEXT_EVENT_DESC]->AddText("Videopalaveri\nasiakkaan kannsa\njatkuu");

	// next event time label
	lbls[LABEL_NEXT_EVENT_TIME] = new EPDGUI_Textbox(701, 370, 231, 106, 3, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_TIME]);
	lbls[LABEL_NEXT_EVENT_TIME]->AddText("18.00 -\n21.00");

	// current event creator label
	lbls[LABEL_CURRENT_EVENT_CREATOR] = new EPDGUI_Textbox(80, 264, 412, 40, 15, 0, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_CREATOR]);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->AddText("Kalle Kallenen");

	// current event desc label
	lbls[LABEL_CURRENT_EVENT_DESC] = new EPDGUI_Textbox(80, 297, 412, 40, 15, 0, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_DESC]);
	lbls[LABEL_CURRENT_EVENT_DESC]->AddText("Videopalaveri");

	// current event time label
	lbls[LABEL_CURRENT_EVENT_TIME] = new EPDGUI_Textbox(80, 330, 412, 53, 15, 0, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_TIME]);
	lbls[LABEL_CURRENT_EVENT_TIME]->AddText("18:00 - 21:00");

	// current event creator label
	lbls[LABEL_CONFIRM_BOOKING] = new EPDGUI_Textbox(144, 164, 310, 40, 0, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_BOOKING]);
	lbls[LABEL_CONFIRM_BOOKING]->AddText("Vahvista varaus");

	// current event desc label
	lbls[LABEL_CONFIRM_FREE] = new EPDGUI_Textbox(144, 209, 310, 40, 0, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_FREE]);
	lbls[LABEL_CONFIRM_FREE]->AddText("Vahvista varauksen poisto");

	// current event time label
	lbls[LABEL_CONFIRM_TIME] = new EPDGUI_Textbox(144, 244, 456, 53, 0, 15, 24, false);
	EPDGUI_AddObject(lbls[LABEL_CONFIRM_TIME]);
	lbls[LABEL_CONFIRM_TIME]->AddText("18:00 - 21:00");
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
	canvasCurrentEvent.fillCanvas(0);

	canvasNextEvent.createCanvas(308, 540);
	canvasNextEvent.fillCanvas(3);

	createButtons();
	createLabels();
	updateStatus();
	guimyTZ->setEvent(updateStatus, guimyTZ->now()+UPDATE_INTERVAL);
}