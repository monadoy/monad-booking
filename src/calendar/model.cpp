#include "model.h"

namespace cal {
Model::Model(APITask& apiTask, SafeTimezone& tz, SafeTimezone& utc)
    : _apiTask{apiTask}, _tz{tz}, _utc{utc} {
	using namespace std::placeholders;
	_apiTask.callbackCalendarStatus = std::bind(&Model::_onCalendarStatus, this, _1);
	_apiTask.callbackEndEvent = std::bind(&Model::_onEndEvent, this, _1);
	_apiTask.callbackInsertEvent = std::bind(&Model::_onInsertEvent, this, _1);
}

void Model::reserveEvent(int minutes) {
	std::lock_guard<std::mutex> lock(_statusMutex);

	const time_t now = _utc.now();

	if (_status->currentEvent) {
		// Current event already exists, can't reserve
		// TODO: send error to GUI
		return;
	}

	time_t endTime = _utc.now() + minutes * SECS_PER_MIN;

	// Round up to five minute
	const time_t remainder = endTime % (5 * SECS_PER_MIN);
	endTime = endTime - remainder + 5 * SECS_PER_MIN;

	// Make fit before next event if needed
	if (_status->nextEvent)
		endTime = min(endTime, _status->nextEvent->unixStartTime);

	if (endTime < now + 30) {
		// If we would create an event that is shorter than 30 seconds,
		// just abort as this event would be useless
		// TODO: send error to GUI
		return;
	}

	_apiTask.insertEvent(now, endTime);
}

void Model::reserveEventUntilNext() {}

void Model::endCurrentEvent() {}

void Model::updateStatus() {}

void Model::_onCalendarStatus(const Result<CalendarStatus>& result) {
	if (result.isErr()) {
		return;
		// TODO: send error to GUI
	}

	auto newStatus = result.ok();

	std::lock_guard<std::mutex> lock(_statusMutex);

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

void Model::_onEndEvent(const Result<Event>& result) {}
void Model::_onInsertEvent(const Result<Event>& result) {}

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
