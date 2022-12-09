#include "apiTask.h"

#include "WiFi.h"
#include "globals.h"

#define API_QUEUE_LENGTH 10
#define API_TASK_PRIORITY 5
#define API_TASK_STACK_SIZE 8192
#define API_TASK_WIFI_CONNECT_MAX_RETRIES 7
#define API_TASK_AUTH_MAX_RETRIES 3

namespace cal {
void task(void* arg) {
	APITask* apiTask = static_cast<APITask*>(arg);

	for (;;) {
		void* reqTemp;
		xQueueReceive(apiTask->_queueHandle, &reqTemp, portMAX_DELAY);
		auto count = sleepManager.scopedTaskCount();
		auto req = toSmartPtr<APITask::QueueElement>(reqTemp);

		auto startTime = millis();

		// TODO: return different error when wifi connection fails (don't leak memory of req->func)
		wifiManager.waitWiFi();

		// TODO: return error when auth refresh fails (don't leak memory of req->func)
		for (int i = 0; i < API_TASK_AUTH_MAX_RETRIES; ++i) {
			if (apiTask->_api->refreshAuth())
				break;
			delay(200);
		}

		switch (req->type) {
			case APITask::RequestType::CALENDAR_STATUS: {
				auto func = toSmartPtr<APITask::QueueFuncCalendarStatus>(req->func);
				apiTask->callbackCalendarStatus((*func)());
				break;
			}
			case APITask::RequestType::END_EVENT: {
				auto func = toSmartPtr<APITask::QueueFuncEvent>(req->func);
				apiTask->callbackEndEvent((*func)());
				break;
			}
			case APITask::RequestType::INSERT_EVENT: {
				auto func = toSmartPtr<APITask::QueueFuncEvent>(req->func);
				apiTask->callbackInsertEvent((*func)());
				break;
			}
			case APITask::RequestType::RESCHEDULE_EVENT: {
				auto func = toSmartPtr<APITask::QueueFuncEvent>(req->func);
				apiTask->callbackRescheduleEvent((*func)());
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
	enqueue(RequestType::END_EVENT, new QueueFuncEvent([=]() { return _api->endEvent(eventId); }));
}

void APITask::insertEvent(time_t startTime, time_t endTime) {
	enqueue(RequestType::INSERT_EVENT,
	        new QueueFuncEvent([=]() { return _api->insertEvent(startTime, endTime); }));
}

void APITask::rescheduleEvent(std::shared_ptr<Event> event, time_t newStartTime,
                              time_t newEndTime) {
	enqueue(RequestType::RESCHEDULE_EVENT, new QueueFuncEvent([=]() {
		        return _api->rescheduleEvent(event, newStartTime, newEndTime);
	        }));
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