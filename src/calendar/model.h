#ifndef CALENDAR_MODEL_H
#define CALENDAR_MODEL_H

#include "apiTask.h"
#include "safeTimezone.h"
#include "utils.h"

namespace cal {

class GUITask;

class Model {
  public:
	Model(APITask& apiTask);

	void registerGUITask(GUITask* task) { _guiTask = task; };

	struct ReserveParams {
		int startTime;
		int endTime;
	};

	/**
	 * Reserve event, use parameters from calculateReserveParams.
	 */
	void reserveEvent(const ReserveParams& params);

	/**
	 * Calculate when to stop and start event when reserving.
	 * Rounds up to the nearest five minutes as long as it doesn't overlap with the next event.
	 */
	utils::Result<ReserveParams> calculateReserveParams(int reserveSeconds);

	/**
	 * Calculate when to stop and start event when reserving.
	 * Rounds up to the nearest five minutes as long as it doesn't overlap with the next event.
	 */
	utils::Result<ReserveParams> calculateReserveUntilNextParams();

	/**
	 * Set the current event to end now.
	 */
	void endCurrentEvent();

	/**
	 * Fetch the current status of the calendar.
	 */
	void updateStatus();

  private:
	void _onCalendarStatus(const Result<CalendarStatus>& result);
	void _onEndEvent(const Result<Event>& result);
	void _onInsertEvent(const Result<Event>& result);

	bool _areEqual(std::shared_ptr<Event> event1, std::shared_ptr<Event> event2) const;

	// Protects _status;
	std::mutex _statusMutex;
	// Remember to protect with mutex as multiple tasks call functions of Model.
	std::shared_ptr<CalendarStatus> _status = std::make_shared<CalendarStatus>();

	APITask& _apiTask;
	GUITask* _guiTask = nullptr;
};
}  // namespace cal

#endif