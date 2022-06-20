#include "model.h"

#include <esp_log.h>

#include "globals.h"
#include "gui/gui.h"

namespace cal {

typedef gui::GUITask::GuiRequest GuiReq;

template <typename T>
bool Model::_handleError(size_t reqType, const Result<T>& result) {
	if (result.isErr()) {
		log_e("%s", result.err()->message.c_str());
		_guiTask->error((GuiReq)reqType, *result.err());
		return true;
	}
	return false;
}

Model::Model(APITask& apiTask) : _apiTask{apiTask} {
	using namespace std::placeholders;
	_apiTask.callbackCalendarStatus = std::bind(&Model::_onCalendarStatus, this, _1);
	_apiTask.callbackEndEvent = std::bind(&Model::_onEndEvent, this, _1);
	_apiTask.callbackInsertEvent = std::bind(&Model::_onInsertEvent, this, _1);

	sleepManager.registerCallback(SleepManager::Callback::BEFORE_SLEEP, [this]() {
		time_t now = safeUTC.now();
		if (_nextStatusUpdate <= now) {
			updateStatus();  // This will cancel sleep
		}
	});

	sleepManager.registerCallback(SleepManager::Callback::AFTER_WAKE_TIMER, [this]() {
		time_t now = safeUTC.now();
		// Take into account inaccuracies in sleep time with -2
		if (_nextStatusUpdate >= now - 2) {
			updateStatus();
		};
	});
}

void Model::reserveEvent(const ReserveParams& params) {
	_apiTask.insertEvent(params.startTime, params.endTime);
}

utils::Result<Model::ReserveParams> Model::calculateReserveParams(int reserveSeconds) {
	std::lock_guard<std::mutex> lock(_statusMutex);

	log_i("Reserving event.");

	const time_t now = safeUTC.now();

	if (_status->currentEvent) {
		log_e("Current event already exists, can't insert another one.");
		return utils::Result<ReserveParams>::makeErr(
		    new utils::Error("Current event already exists, can't insert another one."));
	}

	time_t endTime = now + reserveSeconds;

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
	return calculateReserveParams(_status->nextEvent->unixStartTime - safeUTC.now());
}

void Model::_onInsertEvent(const Result<Event>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Got event insert response.");

	if (_handleError((size_t)GuiReq::RESERVE, result))
		return;

	_status->currentEvent = result.ok();

	_guiTask->success(GuiReq::RESERVE, _status);
}

void Model::endCurrentEvent() {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Ending current event.");
	if (!_status->currentEvent) {
		log_e("No current event exists to end");
		_guiTask->error(GuiReq::FREE, Error("No current event exists to end"));
		return;
	}

	if (_status->currentEvent->unixEndTime <= safeUTC.now()) {
		log_e("Won't end current event as it already ended");
		_guiTask->error(GuiReq::FREE, Error("Won't end current event as it already ended"));
		return;
	}

	_apiTask.endEvent(_status->currentEvent->id);
}

void Model::_onEndEvent(const Result<Event>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Got end event response.");

	if (_handleError((size_t)GuiReq::FREE, result))
		return;

	_status->currentEvent = nullptr;

	_guiTask->success(GuiReq::FREE, _status);
}

void Model::updateStatus() {
	log_i("Fetching calendar status.");
	_nextStatusUpdate = safeUTC.now() + STATUS_UPDATE_INTERVAL_S;
	sleepManager.nextWakeTime = _nextStatusUpdate;
	_apiTask.fetchCalendarStatus();
}

void Model::_onCalendarStatus(const Result<CalendarStatus>& result) {
	std::lock_guard<std::mutex> lock(_statusMutex);
	log_i("Received calendar status.");

	if (_handleError((size_t)GuiReq::UPDATE, result))
		return;

	auto newStatus = result.ok();

	bool changed = !_areEqual(_status->currentEvent, newStatus->currentEvent)
	               || !_areEqual(_status->nextEvent, newStatus->nextEvent)
	               || _status->name != newStatus->name;

	// Don't send an update to GUI if nothing changed
	if (!changed) {
		_guiTask->success(GuiReq::UPDATE, nullptr);
		return;
	}

	_status = newStatus;

	_guiTask->success(GuiReq::UPDATE, _status);
}

bool Model::_areEqual(std::shared_ptr<Event> event1, std::shared_ptr<Event> event2) const {
	if (!!event1 != !!event2)
		return false;
	if (!event1 && !event2)
		return true;
	if (*event1 == *event2)
		return true;
	return false;
}
}  // namespace cal
