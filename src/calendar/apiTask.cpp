#include "apiTask.h"

#include "WiFi.h"
#include "apiTaskEvents.h"

#define API_QUEUE_LENGTH 10
#define API_TASK_PRIORITY 5
#define API_TASK_STACK_SIZE 8192

namespace cal {

void task(void* arg) {
	APITask* apiTask = static_cast<APITask*>(arg);

	for (;;) {
		APITask::QueueElement* req;
		xQueueReceive(apiTask->_queueHandle, &req, portMAX_DELAY);

		if (!WiFi.isConnected()) {
			// TODO: ask for wifi
			WiFi.reconnect();
		}
		while (!WiFi.isConnected()) {
			delay(20);
			Serial.println(WiFi.status());
		}

		apiTask->_api->refreshAuth();

		switch (req->type) {
			case calevents::REQUEST_CALENDAR_STATUS: {
				auto res = apiTask->_api->fetchCalendarStatus();
				esp_event_post(calevents::RESPONSE, calevents::RESPONSE_CALENDAR_STATUS,
				               new calevents::ResponseCalendarStatusData{res},
				               sizeof(calevents::ResponseCalendarStatusData*), 0);
				break;
			}
				// case APITask::ReqType::END:
				// case APITask::ReqType::INSERT:
				// case APITask::ReqType::SIZE:
			default:
				Serial.println("APITask: Unhandled request type " + String(req->type));
		}

		if (req->params != nullptr) {
			delete req->params;
		}
		delete req;
	}

	vTaskDelete(NULL);
}

void eventHandler(void* arg, esp_event_base_t base, int32_t id, void* eventData) {
	APITask* apiTask = static_cast<APITask*>(arg);

	APITask::QueueElement* data = new APITask::QueueElement{(calevents::Request)id, eventData};

	Serial.println("API Task: enqueueing item");
	xQueueSend(apiTask->_queueHandle, (void*)&data, 0);
}

APITask::APITask(std::unique_ptr<API>&& api) : _api{std::move(api)} {
	xTaskCreate(task, "API Task", API_TASK_STACK_SIZE, static_cast<void*>(this), API_TASK_PRIORITY,
	            &_taskHandle);
	_queueHandle = xQueueCreate(API_QUEUE_LENGTH, sizeof(APITask::QueueElement*));

	esp_event_handler_register(calevents::REQUEST, ESP_EVENT_ANY_ID, eventHandler,
	                           static_cast<void*>(this));
}

}  // namespace cal