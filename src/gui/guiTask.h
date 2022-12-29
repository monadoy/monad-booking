#ifndef GUI_TASK_H
#define GUI_TASK_H

#include <Arduino.h>

#include "calendar/model.h"
#include "elements/animation.h"
#include "elements/button.h"
#include "elements/text.h"
#include "gui/gui.h"

namespace gui {

/**
 * Freertos task for controlling the GUI.
 * Most functions can be can be called from any thread,
 * as they are put to the end of the task queue and executed later.
 *
 * Some functions are documented as synchronous and must be called
 * from the qui thread or at specific points in initialization.
 */
class GUITask {
  public:
	GUITask();

	// Created on construction
	GUI _gui;

	enum class Request { RESERVE, FREE, MODEL, UPDATE, OTHER, SIZE };
	static String errorEnumToString(GUITask::Request type);

	QueueHandle_t _queueHandle;
	struct QueueElement {
		QueueElement(void* func) : func{func} {}
		void* func;
	};
	using QueueFunc = std::function<void()>;

	/**
	 * Initialize the main screen and connect model.
	 * This is done synchronously.
	 */
	void initMain(cal::Model* model);

	/**
	 * @brief Called when an operation is executed successfully
	 *
	 * @param type What kind of operation was executed
	 * @param status Current calendar status
	 */
	void success(Request type, std::shared_ptr<cal::CalendarStatus> status);

	/**
	 * @brief Called when an operation caused error
	 *
	 * @param type What kind of operation caused the error
	 * @param error Error to show
	 */
	void error(Request type, std::shared_ptr<cal::Error> error);

	void touchDown(const tp_finger_t& tp);
	void touchUp();

	/**
	 * Close dialogs and put screen to sleep. This way we don't waste energy.
	 */
	void sleep();

	void startLoading();
	void setLoadingScreenText(const String& data);
	void stopLoading();
	void loadingAnimNextFrame();

	/**
	 * Start setup mode. Set useAP to start an access point.
	 */
	void startSetup(bool useAP);

	void showShutdownScreen(const String& shutdownText, bool isError);

  private:
	TaskHandle_t _taskHandle;
	void _enqueue(void* func);
};

}  // namespace gui

#endif