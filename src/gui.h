#ifndef __GUI_H_
#define __GUI_H_

#include <M5EPD.h>

#include <ezTime.h>

#include <epdgui_base.h>
#include <epdgui_button.h>
#include <epdgui_textbox.h>

void EPDGUI_AddObject(EPDGUI_Base* object);
void EPDGUI_Draw(m5epd_update_mode_t mode = UPDATE_MODE_GC16);
void EPDGUI_Process(void);
void EPDGUI_Process(int16_t x, int16_t y);
void EPDGUI_Clear(void);

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
void showFreeBooking();
void hideFreeBooking();
void loadNextBooking();
void loadNextFree();
void hideCurrentBookingLabels(bool isHide);
void loadCurrentBooking();
void loadCurrentFree();
void toConfirmBooking(uint16_t time, bool isTillNext);
void toFreeBooking();
void makeBooking(uint16_t time);
void deleteBooking();

void toMainScreen();

void settingsButton(epdgui_args_vector_t &args);
void fifteenButton(epdgui_args_vector_t &args);
void thirtyButton(epdgui_args_vector_t &args);
void sixtyButton(epdgui_args_vector_t &args);
void ninetyButton(epdgui_args_vector_t &args);
void tillNextButton(epdgui_args_vector_t &args);
void confirmBookingButton(epdgui_args_vector_t &args);
void cancelBookingButton(epdgui_args_vector_t &args);
void confirmFreeButton(epdgui_args_vector_t &args);
void cancelFreeButton(epdgui_args_vector_t &args);
void freeRoomButton(epdgui_args_vector_t &args);
void hideLoading(bool isHide);

void createButtons();
void createRegularLabels();
void createBoldLabels();



void initGui(Timezone *_myTZ);
void loopGui();
void debug(String err);
void updateGui();

#endif