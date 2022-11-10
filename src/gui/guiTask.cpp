#include "guiTask.h"

#include "globals.h"

#define GUI_QUEUE_LENGTH 40
#define GUI_TASK_PRIORITY 5
#define GUI_TASK_STACK_SIZE 4096

namespace gui {

void task(void* arg) {
	GUITask* guiTask = static_cast<GUITask*>(arg);

	log_i("GUI Task created");

	for (;;) {
		void* reqTemp;
		xQueueReceive(guiTask->_queueHandle, &reqTemp, portMAX_DELAY);
		auto counter = sleepManager.scopedTaskCount();
		auto req = toSmartPtr<GUITask::QueueElement>(reqTemp);
		auto func = toSmartPtr<GUITask::QueueFunc>(req->func);
		(*func)();

		// Fix "watchdog triggered" crash by giving some processing time to idle tasks
		delay(2);
	}

	vTaskDelete(NULL);
}

GUITask::GUITask() : _gui(this) {
	using namespace std::placeholders;
	M5.TP.onTouch(std::bind(&GUITask::enqueueTouchDown, this, _1),
	              std::bind(&GUITask::enqueueTouchUp, this));

	_queueHandle = xQueueCreate(GUI_QUEUE_LENGTH, sizeof(GUITask::QueueElement*));
	xTaskCreatePinnedToCore(task, "GUI", GUI_TASK_STACK_SIZE, static_cast<void*>(this),
	                        GUI_TASK_PRIORITY, &_taskHandle, 0);

	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SLEEP,
	                              [this]() { enqueueSleep(); });

	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SHUTDOWN, [this]() {
		// This shouldShutdown call is ignored if an error is already displayed
		time_t projectedTurnOnTime = sleepManager.calculateTurnOnTimeUTC(safeMyTZ.now());
		enqueueShutdownScreen("Shut down. Waking up at "
		                      + safeMyTZ.dateTime(projectedTurnOnTime, UTC_TIME, RFC3339) + ".");
	});
}

void GUITask::initMain(cal::Model* model) { _gui.initMain(model); }

void GUITask::enqueueSetupScreen() {}

void GUITask::enqueueSuccess(Request type, std::shared_ptr<cal::CalendarStatus> status) {
	_enqueue(new QueueFunc([=]() { _gui.setCalendarStatus(status); }));
}

void GUITask::enqueueError(Request type, const cal::Error& error) {}

void GUITask::enqueueTouchDown(const tp_finger_t& tp) {
	_enqueue(new QueueFunc([=]() { _gui.handleTouch(tp.x, tp.y); }));
}
void GUITask::enqueueTouchUp() {
	_enqueue(new QueueFunc([=]() { _gui.handleTouch(-1, -1); }));
}

void GUITask::enqueueSleep() {
	_enqueue(new QueueFunc([=]() { _gui.sleep(); }));
}

void GUITask::enqueueStartLoading() {
	_enqueue(new QueueFunc([=]() { _gui.startLoading(); }));
}
void GUITask::enqueueStopLoading() {
	_enqueue(new QueueFunc([=]() { _gui.stopLoading(); }));
}
void GUITask::enqueueLoadingAnimNextFrame() {
	_enqueue(new QueueFunc([=]() { _gui.showLoadingAnimNextFrame(); }));
}

void GUITask::enqueueSetLoadingScreenText(String data) {
	_enqueue(new QueueFunc([=]() { _gui.setLoadingScreenText(data); }));
}

void GUITask::enqueueShutdownScreen(String shutdownText) {}

void GUITask::_enqueue(void* func) {
	QueueElement* data = new QueueElement{func};
	xQueueSend(_queueHandle, (void*)&data, 0);
}

}  // namespace gui