#ifndef API_TASK_H
#define API_TASK_H

#include <ArduinoJson.h>
#include <esp_event.h>
#include <ezTime.h>

#include <memory>

#include "api.h"

namespace cal {

class APITask {
  public:
	APITask(std::unique_ptr<API>&& api);

	enum RequestType { CALENDAR_STATUS, END_EVENT, INSERT_EVENT };
	struct QueueElement {
		QueueElement(RequestType t, void* func) : type{t}, func{func} {}
		RequestType type;
		void* func;
	};
	using QueueFuncCalendarStatus = std::function<Result<CalendarStatus>()>;
	using QueueFuncEndEvent = std::function<Result<Event>()>;
	using QueueFuncInsertEvent = std::function<Result<Event>()>;

	// Callback must be set before calling
	void fetchCalendarStatus();
	std::function<void(Result<CalendarStatus>)> callbackCalendarStatus;

	// Callback must be set before calling
	void endEvent(const String& eventId);
	std::function<void(Result<Event>)> callbackEndEvent;

	// Callback must be set before calling
	void insertEvent(time_t startTime, time_t endTime);
	std::function<void(Result<Event>)> callbackInsertEvent;

	const std::unique_ptr<API> _api;
	QueueHandle_t _queueHandle;

  private:
	void enqueue(RequestType rt, void* func) {
		QueueElement* data = new QueueElement{rt, func};
		xQueueSend(_queueHandle, (void*)&data, 0);
	};
	TaskHandle_t _taskHandle;
};  // namespace cal

}  // namespace cal

namespace {

/*
 * Casts pointer to type and wraps it in a unique_ptr for safe RAII deletion.
 */
template <typename T>
std::unique_ptr<T> toSmartPtr(void* ptr) {
	return std::unique_ptr<T>(static_cast<T*>(ptr));
}

}  // namespace

#endif