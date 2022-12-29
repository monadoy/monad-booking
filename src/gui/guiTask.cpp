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
		delay(5);
	}

	vTaskDelete(NULL);
}

GUITask::GUITask() : _gui(this) {
	using namespace std::placeholders;
	M5.TP.onTouch(std::bind(&GUITask::touchDown, this, _1), std::bind(&GUITask::touchUp, this));

	_queueHandle = xQueueCreate(GUI_QUEUE_LENGTH, sizeof(GUITask::QueueElement*));
	xTaskCreatePinnedToCore(task, "GUI", GUI_TASK_STACK_SIZE, static_cast<void*>(this),
	                        GUI_TASK_PRIORITY, &_taskHandle, 0);

	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SLEEP, [this]() { sleep(); });

	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SHUTDOWN, [this]() {
		// This shouldShutdown call is ignored if an error is already displayed
		time_t projectedTurnOnTime = sleepManager.calculateTurnOnTimeUTC(safeMyTZ.now());
		showShutdownScreen("Shut down.\nWaking up at "
		                   + safeMyTZ.dateTime(projectedTurnOnTime, UTC_TIME, RFC3339) + ".", false);
	});
}

void GUITask::initMain(cal::Model* model) { _gui.initMain(model); }

void GUITask::startSetup(bool useAP) { _gui.startSetup(useAP); }

void GUITask::success(Request type, std::shared_ptr<cal::CalendarStatus> status) {
	_enqueue(new QueueFunc([=]() { _gui.showCalendarStatus(status); }));
}

String GUITask::errorEnumToString(GUITask::Request type) {
	switch (type) {
		case GUITask::Request::RESERVE:
			return "RESERVE";
		case GUITask::Request::FREE:
			return "FREEING";
		case GUITask::Request::MODEL:
			return "MODEL";
		case GUITask::Request::UPDATE:
			return "UPDATE";
		default:
			return "OTHER";
	}
}

void GUITask::error(Request type, std::shared_ptr<cal::Error> error) {
	if (!error)
		return;

	String errorStr = errorEnumToString(type) + " ERROR:\n" + error->message;

	_enqueue(new QueueFunc([=]() { _gui.showError(errorStr); }));
}

void GUITask::touchDown(const tp_finger_t& tp) {
	_enqueue(new QueueFunc([=]() { _gui.handleTouch(tp.x, tp.y); }));
}
void GUITask::touchUp() {
	_enqueue(new QueueFunc([=]() { _gui.handleTouch(-1, -1); }));
}

void GUITask::sleep() {
	_enqueue(new QueueFunc([=]() { _gui.sleep(); }));
}

void GUITask::startLoading() {
	_enqueue(new QueueFunc([=]() { _gui.startLoading(); }));
}
void GUITask::stopLoading() {
	_enqueue(new QueueFunc([=]() { _gui.stopLoading(); }));
}
void GUITask::loadingAnimNextFrame() {
	_enqueue(new QueueFunc([=]() { _gui.showLoadingAnimNextFrame(); }));
}

void GUITask::setLoadingScreenText(const String& data) {
	_enqueue(new QueueFunc([=]() { _gui.setLoadingScreenText(data); }));
}

void GUITask::showShutdownScreen(const String& shutdownText, bool isError) {
	_enqueue(new QueueFunc([=]() { _gui.showShutdownScreen(shutdownText, isError); }));
}

void GUITask::_enqueue(void* func) {
	QueueElement* data = new QueueElement{func};
	xQueueSend(_queueHandle, (void*)&data, 0);
}

}  // namespace gui