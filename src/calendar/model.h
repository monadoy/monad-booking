#ifndef CALENDAR_MODEL_H
#define CALENDAR_MODEL_H

#include "apiTask.h"
#include "safeTimezone.h"
#include "utils.h"

namespace gui {
class GUITask;
}  // namespace gui

namespace cal {

/**
 * Models the state of the calendar. Works as glue between the APITask and GUI.
 */
class Model {
  public:
	Model(APITask& apiTask);

	void registerGUITask(gui::GUITask* task) { _guiTask = task; };

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
	 * Set the current event to end n seconds later than currently.
	 */
	void extendCurrentEvent(int seconds);

	/**
	 * Fetch the current status of the calendar.
	 * Also updates sleep timings in sleep manager.
	 */
	void updateStatus();

  private:
	void _onCalendarStatus(const Result<CalendarStatus>& result);
	void _onEndEvent(const Result<Event>& result);
	void _onInsertEvent(const Result<Event>& result);
	void _onExtendEvent(const Result<Event>& result);

	bool _areEqual(std::shared_ptr<Event> event1, std::shared_ptr<Event> event2) const;

	/**
	 * Does required operations for model errors.
	 * Does:
	 * - logging
	 * - asynchronously notify GUI about an error
	 * - update state if its type is LOGICAL,
	 *   because the error was probably caused by out of date information
	 */
	void _handleError(size_t reqType, std::shared_ptr<Error> error);

	/**
	 * Creates a result based on supplied error and does required operations.
	 * We do:
	 * - logging
	 * - update state because it was probably out of date
	 */
	template <typename T>
	utils::Result<T> _handleStateErrorSync(utils::Error* error);

	// Protects _status;
	std::mutex _statusMutex;
	// Remember to protect with mutex as multiple tasks call functions of Model.
	std::shared_ptr<CalendarStatus> _status = std::make_shared<CalendarStatus>();

	// Unix UTC seconds
	time_t _nextStatusUpdate = 0;

	APITask& _apiTask;
	gui::GUITask* _guiTask = nullptr;
};
}  // namespace cal

#endif