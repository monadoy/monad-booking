#include "apiTask.h"

#include "WiFi.h"

#define API_QUEUE_LENGTH 10
#define API_TASK_PRIORITY 5
#define API_TASK_STACK_SIZE 8192 * 8

namespace cal {

void task(void* arg) {
	APITask* apiTask = static_cast<APITask*>(arg);

	Serial.print("API Task created");

	for (;;) {
		void* reqTemp;
		Serial.print("API Task: waiting for queue receive");
		xQueueReceive(apiTask->_queueHandle, &reqTemp, portMAX_DELAY);
		auto req = toSmartPtr<APITask::QueueElement>(reqTemp);
		Serial.print("API Task: queue item received");

		if (!WiFi.isConnected()) {
			// TODO: Find a better way to ask for wifi
			WiFi.reconnect();
		}
		while (!WiFi.isConnected()) {
			delay(20);
			Serial.println(WiFi.status());
		}

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
				Serial.println("APITask: Unhandled request type " + String((uint8_t)req->type));
				break;
		}
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
	xTaskCreate(task, "API Task", API_TASK_STACK_SIZE, static_cast<void*>(this), API_TASK_PRIORITY,
	            &_taskHandle);

	_queueHandle = xQueueCreate(API_QUEUE_LENGTH, sizeof(APITask::QueueElement*));

	if (_queueHandle != NULL) {
		Serial.println("API Task: queue created");
	} else {
		Serial.println("API Task: queue create failed");
	}
}

}  // namespace cal