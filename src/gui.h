#ifndef __GUI_H_
#define __GUI_H_

#include <epdgui_base.h>
#include <epdgui_button.h>
#include <epdgui_textbox.h>
#include <ezTime.h>

#include "configServer.h"

namespace {

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
    BUTTON_CONTINUE,
    BUTTON_SETUP,
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
	LABEL_ERROR,
	LABEL_SETTINGS_STARTUP,
	LABEL_SIZE
};

const uint16_t FONT_SIZE_NORMAL = 24;
const uint16_t FONT_SIZE_BUTTON = 28;
const uint16_t FONT_SIZE_HEADER = 32;
const uint16_t FONT_SIZE_CLOCK = 44;
const uint16_t FONT_SIZE_TITLE = 64;

enum { SCREEN_MAIN, SCREEN_BOOKING, SCREEN_FREEING, SCREEN_SETTINGS, SCREEN_SIZE };

void EPDGUI_AddObject(EPDGUI_Base* object);
void EPDGUI_Draw(m5epd_update_mode_t mode = UPDATE_MODE_GC16);
void EPDGUI_Process(void);
void EPDGUI_Process(int16_t x, int16_t y);

String getBatteryPercent();
String getWifiStatus();
void updateStatus();
void updateScreen();
void updateClocksWifiBattery();
void hideNextBooking(bool isHide);
void configureMainButtonPos();
void hideMainButtons(bool isHide);
time_t roundToFive(time_t endTime);
void hideBookingConfirmationButtons(bool isHide);
void hideFreeConfirmationButtons(bool isHide);
void hideFreeRoomButton(bool isHide);
void showConfirmBooking(uint16_t time);
void hideMainLabels(bool isHide);
void hideConfirmBooking();
void hideFreeBooking(bool isHide);
void loadNextBooking();
void loadNextFree();
void hideCurrentBookingLabels(bool isHide);
void loadCurrentBooking();
void loadCurrentFree();
void toConfirmBooking(uint16_t time, bool isTillNext);
void toFreeBooking();
void makeBooking(uint16_t time);
void deleteBooking();
void hideSettings(bool isHide);

void toMainScreen();
void toSettingsScreen();

void settingsButton(epdgui_args_vector_t& args);
void fifteenButton(epdgui_args_vector_t& args);
void thirtyButton(epdgui_args_vector_t& args);
void sixtyButton(epdgui_args_vector_t& args);
void ninetyButton(epdgui_args_vector_t& args);
void tillNextButton(epdgui_args_vector_t& args);
void confirmBookingButton(epdgui_args_vector_t& args);
void cancelBookingButton(epdgui_args_vector_t& args);
void confirmFreeButton(epdgui_args_vector_t& args);
void cancelFreeButton(epdgui_args_vector_t& args);
void freeRoomButton(epdgui_args_vector_t& args);
void continueButton(epdgui_args_vector_t& args);
void setupButton(epdgui_args_vector_t& args);
void hideLoading(bool isHide);

void createButtons();
void createRegularLabels();
void createBoldLabels();
}

namespace gui {
void initGui(Timezone* _myTZ, Config::ConfigStore* configStore);
void loopGui();
void debug(String err);
void clearDebug();
void updateGui();
} // namespace gui
#endif