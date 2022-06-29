#include "apiTask.h"

#include "WiFi.h"
#include "globals.h"

#define API_QUEUE_LENGTH 10
#define API_TASK_PRIORITY 5
#define API_TASK_STACK_SIZE 8192
#define API_TASK_WIFI_CONNECT_MAX_RETRIES 1

namespace cal {
void task(void* arg) {
	APITask* apiTask = static_cast<APITask*>(arg);

	for (;;) {
		void* reqTemp;
		xQueueReceive(apiTask->_queueHandle, &reqTemp, portMAX_DELAY);
		auto count = sleepManager.scopedTaskCount();
		auto req = toSmartPtr<APITask::QueueElement>(reqTemp);

		auto startTime = millis();

		wifiManager.waitWiFi(API_TASK_WIFI_CONNECT_MAX_RETRIES);

		apiTask->_api->refreshAuth();

		switch (req->type) {
			case APITask::RequestType::CALENDAR_STATUS: {
				auto func = toSmartPtr<APITask::QueueFuncCalendarStatus>(req->func);
				apiTask->callbackCalendarStatus((*func)());
				break;
			}
			case APITask::RequestType::END_EVENT: {
				auto func = toSmartPtr<APITask::QueueFuncEndEvent>(req->func);
				apiTask->callbackEndEvent((*func)());
				break;
			}
			case APITask::RequestType::INSERT_EVENT: {
				auto func = toSmartPtr<APITask::QueueFuncInsertEvent>(req->func);
				apiTask->callbackInsertEvent((*func)());
				break;
			}
			default:
				log_e("APITask: Unhandled request type %u", req->type);
				break;
		}

		log_i("Request completed in %u ms.", millis() - startTime);
	}

	vTaskDelete(NULL);
}

void APITask::fetchCalendarStatus() {
	enqueue(RequestType::CALENDAR_STATUS,
	        new QueueFuncCalendarStatus([=]() { return _api->fetchCalendarStatus(); }));
}

void APITask::endEvent(const String& eventId) {
	enqueue(RequestType::END_EVENT,
	        new QueueFuncEndEvent([=]() { return _api->endEvent(eventId); }));
}

void APITask::insertEvent(time_t startTime, time_t endTime) {
	enqueue(RequestType::INSERT_EVENT,
	        new QueueFuncInsertEvent([=]() { return _api->insertEvent(startTime, endTime); }));
}

void APITask::enqueue(RequestType rt, void* func) {
	QueueElement* data = new QueueElement{rt, func};
	xQueueSend(_queueHandle, (void*)&data, 0);
};

APITask::APITask(std::unique_ptr<API>&& api) : _api{std::move(api)} {
	_queueHandle = xQueueCreate(API_QUEUE_LENGTH, sizeof(APITask::QueueElement*));
	assert(_queueHandle != NULL);

	BaseType_t taskCreateRes
	    = xTaskCreatePinnedToCore(task, "API Task", API_TASK_STACK_SIZE, static_cast<void*>(this),
	                              API_TASK_PRIORITY, &_taskHandle, 1);
	assert(taskCreateRes == pdPASS);
}

}  // namespace cal