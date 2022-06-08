#include "apiTask.h"

#include "WiFi.h"
#include "apiTaskEvents.h"

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
}

APITask::APITask(std::unique_ptr<API>&& api) : _api{std::move(api)} {
	xTaskCreate(task, "API Task", 2048, static_cast<void*>(this), 5, &_taskHandle);
	_queueHandle = xQueueCreate(10, sizeof(APITask::QueueElement*));

	esp_event_handler_register(calevents::REQUEST, ESP_EVENT_ANY_ID, eventHandler,
	                           static_cast<void*>(this));
}

}  // namespace cal