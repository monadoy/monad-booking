#ifndef GUI_H
#define GUI_H

#include <M5EPD_Driver.h>
#include <epdgui_base.h>
#include <epdgui_button.h>
#include <epdgui_textbox.h>
#include <ezTime.h>

#include "calendar/api.h"
#include "calendar/model.h"
#include "configServer.h"
#include "safeTimezone.h"
#include "gui/animManager.h"

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
	LABEL_BOOTLOG,
	LABEL_SIZE
};

const uint16_t FONT_SIZE_BOOTLOG = 16;
const uint16_t FONT_SIZE_NORMAL = 24;
const uint16_t FONT_SIZE_BUTTON = 28;
const uint16_t FONT_SIZE_HEADER = 32;
const uint16_t FONT_SIZE_CLOCK = 44;
const uint16_t FONT_SIZE_TITLE = 64;

enum {
	SCREEN_MAIN,
	SCREEN_BOOKING,
	SCREEN_FREEING,
	SCREEN_SETTINGS,
	SCREEN_SETUP,
	SCREEN_BOOTLOG,
	SCREEN_SHUTDOWN,
	SCREEN_STARTUP,
	SCREEN_STARTUP_ERROR,
	SCREEN_SIZE
};

void EPDGUI_AddObject(EPDGUI_Base* object);
void EPDGUI_Draw(EPDGUI_Base* object, m5epd_update_mode_t mode);
void EPDGUI_Draw(m5epd_update_mode_t mode = UPDATE_MODE_NONE);
void EPDGUI_Process(void);
void EPDGUI_Process(int16_t x, int16_t y);

String getBatteryPercent();
String getWifiStatus();
void updateStatus(std::shared_ptr<cal::CalendarStatus> statusCopy);
void updateScreen(bool pushLeft, bool pushRight, m5epd_update_mode_t updateMode = UPDATE_MODE_GC16);
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

void createButton(int ButtonEnum, String label, int16_t x, int16_t y, int16_t h, uint16_t color,
                  uint16_t txt_color, uint16_t color_pressed, bool use_bold,
                  void (*func_cb)(epdgui_args_vector_t&));
void createButtons();
void createRegularLabels();
void createBoldLabels();
void tryToPutSleep();
void debug(String err);
void clearDebug();
void toSleep();
void setLoadingText(String text);

}  // namespace

namespace gui {

class GUITask {
  public:
	GUITask();
	enum class GuiRequest { RESERVE, FREE, MODEL, UPDATE, OTHER, SIZE };

	/* std::array<String, (size_t)GuiRequest::SIZE> guiRequestStrings{"RESERVE", "FREE", "OTHER",
	 * "MODEL"}; */

	QueueHandle_t _guiQueueHandle;
	struct GuiQueueElement {
		GuiQueueElement(void* func) : func{func} {}
		void* func;
	};

	using QueueFunc = std::function<void()>;
	/**
	 * @brief Called when operation is executed successfully
	 *
	 * @param type What kind of operation was executed
	 * @param status Current calendar status
	 */
	void success(GuiRequest type, std::shared_ptr<cal::CalendarStatus> status);

	/**
	 * @brief Called when operation caused error
	 *
	 * @param type What kind of operation caused the error
	 * @param error Error to show
	 */
	void error(GuiRequest type, const cal::Error& error);

	/**
	 * @brief Touch down event
	 *
	 * @param tp struct to hold data from touch press.
	 */
	void touchDown(const tp_finger_t& tp);
	std::function<void(const tp_finger_t&)> callbackTouchDown;

	/**
	 * @brief Touch up event
	 *
	 */
	void touchUp();
	std::function<void()> callbackTouchUp;

	/**
	 * @brief Function to call before gui goes to sleep
	 *
	 */
	void sleep();

	/**
	 * @brief  Initializes the main screen with model pointer
	 *
	 * @param model
	 */
	void initMain(cal::Model* model);

	/**
	 * @brief Starts loading
	 *
	 */
	void startLoading(bool isReverse = false);

	/**
	 * @brief Calls next animation frame to be loaded, recursively calls itself until success or
	 * error appears.
	 *
	 */
	void loadNextFrame(bool isReverse);

	/**
	 * @brief Shows text during loading
	 * 
	 * @param data text to be shown 
	 */
	void showLoadingText(String data);

	/**
	 * @brief Stops loading animation
	 * 
	 */
	void stopLoading();

	/**
	 * @brief Shows shutdownscreen with preferred text
	 * 
	 * @param shutdownText text to be shown under the logo
	 */
	void showShutdown(String shutdownText, bool isBootError = true);

	/**
	 * @brief Puts GUI to setupscreen
	 * 
	 */
	void goSetup(bool fromMain = true);
  private:
	TaskHandle_t _taskHandle;
	void enqueue(void* func);
};
void initGui();
void createSetupButton();
void initMainScreen(cal::Model* model);
void toSetupScreen(bool fromMain = false);
void showBootLog();
void registerModel(cal::Model* model);
void registerAnimation(anim::Animation* loadingAnimation);
void task(void* arg);
void updateGui(gui::GUITask::GuiRequest type, std::shared_ptr<cal::CalendarStatus> status);
void displayError(gui::GUITask::GuiRequest type, const cal::Error& error);
String enumToString(gui::GUITask::GuiRequest type);

}  // namespace gui
#endif