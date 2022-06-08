#ifndef API_TASK_H
#define API_TASK_H

#include <ArduinoJson.h>
#include <esp_event.h>
#include <ezTime.h>

#include <memory>

#include "api.h"
#include "apiTaskEvents.h"

namespace cal {

class APITask {
  public:
	APITask(std::unique_ptr<API>&& api);

	struct QueueElement {
		QueueElement(calevents::Request t, void* p) : type{t}, params{p} {}
		calevents::Request type;
		void* params;
	};

	const std::unique_ptr<API> _api;
	QueueHandle_t _queueHandle;

  private:
	TaskHandle_t _taskHandle;
};

}  // namespace cal
#endif