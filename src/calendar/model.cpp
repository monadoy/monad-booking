#include "model.h"

#include <esp_log.h>

namespace cal {
Model::Model(APITask& apiTask, SafeTimezone& tz, SafeTimezone& utc)
    : _apiTask{apiTask}, _tz{tz}, _utc{utc} {
	using namespace std::placeholders;
	_apiTask.callbackCalendarStatus = std::bind(&Model::_onCalendarStatus, this, _1);
	_apiTask.callbackEndEvent = std::bind(&Model::_onEndEvent, this, _1);
	_apiTask.callbackInsertEvent = std::bind(&Model::_onInsertEvent, this, _1);
}

void Model::reserveEvent(int seconds) {
	std::lock_guard<std::mutex> lock(_statusMutex);

	log_d("Reserving event.");

	const time_t now = _utc.now();

	if (_status->currentEvent) {
		log_e("Current event already exists, can't insert more.");
		// Current event already exists, can't reserve
		// TODO: send error to GUI
		return;
	}

	time_t endTime = _utc.now() + seconds;

	// Round up to five minute
	const time_t remainder = endTime % (5 * SECS_PER_MIN);
	endTime = endTime - remainder + 5 * SECS_PER_MIN;

	// Make fit before next event if needed
	if (_status->nextEvent)
		endTime = min(endTime, _status->nextEvent->unixStartTime);

	if (endTime < now + 30) {
		log_e("Won't insert such a short event (%d seconds).", endTime - now);
		// If we would create an event that is shorter than 30 seconds,
		// just abort as this event would be useless
		// TODO: send error to GUI
		return;
	}

	_apiTask.insertEvent(now, endTime);
}

void Model::reserveEventUntilNext() {
	std::lock_guard<std::mutex> lock(_statusMutex);
	if (!_status->nextEvent) {
		log_e("No next event exists.");
		// TODO: Send error to GUI
		return;
	}
	reserveEvent(_status->nextEvent->unixStartTime - _utc.now());
}

void Model::_onInsertEvent(const Result<Event>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_d("Got event insert response.");

	if (result.isErr()) {
		log_e("%s", result.err()->message.c_str());
		// TODO: send error to GUI
		return;
	}

	_status->currentEvent = result.ok();

	// TODO: send new event to UI
}

void Model::endCurrentEvent() {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_d("Ending current event.");
	if (!_status->currentEvent) {
		log_e("No current event exists to end");
		// TODO: send error to GUI
		return;
	}

	if (!_status->currentEvent->unixEndTime <= _utc.now()) {
		log_e("Won't end current event as it already ended");
		// TODO: send error to GUI
		return;
	}

	_apiTask.endEvent(_status->currentEvent->id);
}

void Model::_onEndEvent(const Result<Event>& result) {
	log_d("Got end event response.");

	if (result.isErr()) {
		log_e("%s", result.err()->message.c_str());
		// TODO: send error to GUI
		return;
	}

	_status->currentEvent = nullptr;

	// TODO: send new non-event to GUI
}

void Model::updateStatus() {
	log_d("Fetching calendar status.");
	_apiTask.fetchCalendarStatus();
}

void Model::_onCalendarStatus(const Result<CalendarStatus>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_d("Received calendar status.");

	if (result.isErr()) {
		log_e("%s", result.err()->message.c_str());
		// TODO: send error to GUI
		return;
	}

	auto newStatus = result.ok();

	bool changed = !_areEqual(_status->currentEvent, newStatus->currentEvent)
	               || !_areEqual(_status->nextEvent, newStatus->nextEvent)
	               || _status->name != newStatus->name;

	// Don't send an update to GUI if nothing changed
	if (!changed)
		return;

	_status = newStatus;

	// TODO: send new status to GUI
	// TODO: what about the situation where nothing changes and no update is sent
	// but GUI should still update clock?
}

bool Model::_areEqual(std::shared_ptr<Event> event1, std::shared_ptr<Event> event2) const {
	if (!!event1 != !!event2) {
		return false;
	}
	if (!event1 && !event2) {
		return true;
	}
	if (*event1 == *event2) {
		return true;
	}
	return false;
}
}  // namespace cal
