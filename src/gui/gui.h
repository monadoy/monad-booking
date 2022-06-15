#ifndef GUI_H
#define GUI_H

#include <epdgui_base.h>
#include <epdgui_button.h>
#include <epdgui_textbox.h>
#include <ezTime.h>
#include <M5EPD_Driver.h>
#include "safeTimezone.h"
#include "calendar/api.h"
#include "calendar/model.h"
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

enum { SCREEN_MAIN, SCREEN_BOOKING, SCREEN_FREEING, SCREEN_SETTINGS, SCREEN_SETUP, SCREEN_BOOTLOG, SCREEN_SIZE };

void EPDGUI_AddObject(EPDGUI_Base* object);
void EPDGUI_Draw(EPDGUI_Base* object, m5epd_update_mode_t mode);
void EPDGUI_Draw(m5epd_update_mode_t mode = UPDATE_MODE_NONE);
void EPDGUI_Process(void);
void EPDGUI_Process(int16_t x, int16_t y);

String getBatteryPercent();
String getWifiStatus();
void updateStatus(const cal::CalendarStatus* status);
void updateScreen(bool pushLeft, bool pushRight);
void updateClocksWifiBattery();
void hideNextBooking(bool isHide);
int configureMainButtonPos(bool isHide);
void hideMainButtons(bool isHide);
void hideBookingConfirmationButtons(bool isHide);
void hideFreeConfirmationButtons(bool isHide);
void hideFreeRoomButton(bool isHide);
void hideConfirmBooking(uint16_t time, bool isHide);
void hideMainLabels(bool isHide);
void hideFreeBooking(bool isHide);
void loadNextBooking();
void loadNextFree();
void hideCurrentBookingLabels(bool isHide);
void loadCurrentBooking();
void loadCurrentFree();
void toConfirmBooking(uint16_t time, bool isTillNext);
void toFreeBooking();
void makeBooking(const cal::Model::ReserveParams& params);
void deleteBooking();
void hideSettings(bool isHide);

void toMainScreen(bool updateLeft, bool updateRight);
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
void tryToPutSleep();
} // namespace

namespace gui {
void registerModel(cal::Model* model); 
void initGui(Config::ConfigStore* configStore);
void loopGui();
void debug(String err);
void clearDebug();
void toSetupScreen();
void showBootLog();

class GUITask {
  public:
	GUITask(Config::ConfigStore* configStore, cal::Model* model);
	enum class ActionType {
		SUCCESS,
		ERROR,
		STATE_UPDATE,
		TOUCH_DOWN,
		TOUCH_UP
	};

	enum class GuiRequest {
		RESERVE,
		FREE,
		OTHER
	};
	QueueHandle_t _queueHandle;
	struct GuiQueueElement {
		GuiQueueElement(ActionType t, void* func) : type{t}, func{func} {}
		ActionType type;
		void* func;
	};
	
	using QueueFunc = std::function<void()>;
	/**
	 * @brief Called when operation is executed successfully
	 * 
	 * @param type What kind of operation was executed
	 * @param status Current calendar status
	 */
	void success(GuiRequest type, cal::CalendarStatus* status);

	/**
	 * @brief Called when operation caused error
	 * 
	 * @param type What kind of operation caused the error
	 * @param error Error to show
	 */
	void error(GuiRequest type, const cal::Error& error);

	/**
	 * @brief Gui has to update current state.
	 * 
	 * @param status nullptr if nothing changed, otherwise changed event
	 */
	void stateChanged(cal::CalendarStatus* status);

	/**
	 * @brief Touch down event
	 * 
	 * @param tp struct to hold data from touch press.
	 */
	void touchDown(const tp_finger_t& tp);

	/**
	 * @brief Touch up event
	 * 
	 */
	void touchUp();

	/**
	 * @brief event to load setup
	 * 
	 */
	void loadSetup();

private:
	TaskHandle_t _taskHandle;
	void enqueue(ActionType at, void* func);
};

} // namespace gui
#endif