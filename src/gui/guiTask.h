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
 * Functions starting with "enqueue" are asynchronous can be called from any thread.
 * They are put to the end of the task queue and executed in later.
 *
 * Other functions are synchronous and must be called from the qui thread or at specific points in
 * initialization.
 */
class GUITask {
  public:
	GUITask();

	// Created on construction
	GUI _gui;

	enum class Request { RESERVE, FREE, MODEL, UPDATE, OTHER, SIZE };
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
	 * Show the setup screen.
	 */
	void enqueueSetupScreen();

	/**
	 * @brief Called when an operation is executed successfully
	 *
	 * @param type What kind of operation was executed
	 * @param status Current calendar status
	 */
	void enqueueSuccess(Request type, std::shared_ptr<cal::CalendarStatus> status);

	/**
	 * @brief Called when an operation caused error
	 *
	 * @param type What kind of operation caused the error
	 * @param error Error to show
	 */
	void enqueueError(Request type, const cal::Error& error);

	void enqueueTouchDown(const tp_finger_t& tp);
	void enqueueTouchUp();

	/**
	 * Close dialogs and put screen to sleep. This way we don't waste energy.
	 */
	void enqueueSleep();

	void enqueueStartLoading();
	void enqueueSetLoadingScreenText(String data);
	void enqueueStopLoading();
	void enqueueLoadingAnimNextFrame();

	void enqueueShutdownScreen(String shutdownText);

  private:
	TaskHandle_t _taskHandle;
	void _enqueue(void* func);
};

}  // namespace gui

#endif