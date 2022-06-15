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

void Model::reserveEvent(const ReserveParams& params) {
	_apiTask.insertEvent(params.startTime, params.endTime);
}

utils::Result<Model::ReserveParams> Model::calculateReserveParams(int reserveSeconds) {
	std::lock_guard<std::mutex> lock(_statusMutex);

	log_i("Reserving event.");

	const time_t now = _utc.now();

	if (_status->currentEvent) {
		log_e("Current event already exists, can't insert another one.");
		return utils::Result<ReserveParams>::makeErr(
		    new utils::Error("Current event already exists, can't insert another one."));
	}

	time_t endTime = _utc.now() + reserveSeconds;

	// Round up to five minute
	const time_t remainder = endTime % (5 * SECS_PER_MIN);
	endTime = endTime - remainder + 5 * SECS_PER_MIN;

	// Make fit before next event if needed
	if (_status->nextEvent)
		endTime = min(endTime, _status->nextEvent->unixStartTime);

	if (endTime < now + 30) {
		log_e("Won't insert such a short event (%d seconds).", endTime - now);
		return utils::Result<ReserveParams>::makeErr(new utils::Error{
		    "Won't insert such a short event (" + String(endTime - now) + " seconds)."});
	}

	return utils::Result<ReserveParams>::makeOk(
	    new ReserveParams{.startTime = now, .endTime = endTime});
}

utils::Result<Model::ReserveParams> Model::calculateReserveUntilNextParams() {
	{
		std::lock_guard<std::mutex> lock(_statusMutex);
		if (!_status->nextEvent) {
			log_e("No next event exists.");
			return utils::Result<ReserveParams>::makeErr(new utils::Error{"No next event exists."});
		}
	}
	return calculateReserveParams(_status->nextEvent->unixStartTime - _utc.now());
}

void Model::_onInsertEvent(const Result<Event>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Got event insert response.");

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
	log_i("Ending current event.");
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
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Got end event response.");

	if (result.isErr()) {
		log_e("%s", result.err()->message.c_str());
		// TODO: send error to GUI
		return;
	}

	_status->currentEvent = nullptr;

	// TODO: send new non-event to GUI
}

void Model::updateStatus() {
	log_i("Fetching calendar status.");
	_apiTask.fetchCalendarStatus();
}

void Model::_onCalendarStatus(const Result<CalendarStatus>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Received calendar status.");

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
