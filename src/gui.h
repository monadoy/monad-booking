#ifndef __GUI_H_
#define __GUI_H_

#include <M5EPD.h>
#include <epdgui.h>
#include <ezTime.h>

String getBatteryPercent();
String getWifiStatus();
void updateStatus();
void updateScreen();
void hideNextBooking(bool isHide);
void hideMainButtons(bool isHide);
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
void toConfirmBooking();
void toFreeBooking();
void makeBooking();
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

void createButtons();
void createLabels();


void initGui(Timezone *_myTZ);

#endif