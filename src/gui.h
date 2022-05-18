#ifndef __GUI_H_
#define __GUI_H_

#include <M5EPD.h>
#include <epdgui.h>
#include <ezTime.h>

void updateScreen();
void hideNextBooking(bool isHide);
void hideMainButtons(bool isHide);
void hideBookingConfirmationButtons(bool isHide);
void hideFreeConfirmationButtons(bool isHide);
void hideFreeRoomButton(bool isHide);
void showConfirmBooking(String time);
void hideMainLabels(bool isHide);
void hideConfirmBooking();
void showFreeBooking(String time);
void hideFreeBooking();
void loadNextBooking(String time, String voltage, String wifiStatus);
void loadNextFree(String time, String voltage, String wifiStatus);
void hideCurrentBookingLabels(bool isHide);
void loadCurrentBooking();
void loadCurrentFree();
void toConfirmBooking();
void toFreeBooking();
void makeBooking();
void deleteBooking();
void makeNextBooking();
void deleteNextBooking();

void ToMainScreen();

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