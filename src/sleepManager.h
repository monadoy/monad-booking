#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H
#include <Arduino.h>
#include <ezTime.h>

#define SLEEP_MANAGER_MAX_TASKS 10

// How long to keep awake after touch event
#define TOUCH_WAKE_TIMEOUT_MS 15 * 1000

// How long the keep awake after tasks are complete.
// This works as a safety buffer when there is some processing time between
// two sequential tasks that should keep us awake.
#define TASK_WAKE_TIMEOUT_MS 200

#define TIMER_CALLBACK_INTERVAL_S 2 * 60

class SleepManager;

class ScopedTaskCounter {
  public:
	ScopedTaskCounter(SleepManager* manager);
	~ScopedTaskCounter();

  private:
	SleepManager* _manager;
};

/**
 * Handles going to sleep and waking up to touch or reccurring timer.
 * Makes us sleep when there are no active tasks and there hasn't been a touch input in some time.
 * Tasks
 */
class SleepManager {
  public:
	SleepManager();

	/**
	 * Convenient RAII task count.
	 * Increments when constructed, decrements when destructed.
	 * YOU NEED TO SAVE THE RESULT OF THIS FUNCTION IN A VARIABLE.
	 */
	[[nodiscard]] ScopedTaskCounter scopedTaskCount();

	/**
	 * You should probably be using scopedTaskCount insted of these.
	 */
	void incrementTaskCounter();
	void decrementTaskCounter();

	/**
	 * Refresh touch wake timer to prevent going to sleep too early.
	 */
	void refreshTouchWake();

	// TODO: callbacks for waking to touch and sleep
	// TODO: shutdown for the night if needed

	void _sleep();
	SemaphoreHandle_t _activeTaskCounter;

	/*
	 * Timer callback that is executed every TIMER_CALLBACK_INTERVAL_S.
	 * Device will wake up from sleep to ensure that the callback is executed in a timely manner.
	 * If already woken, we will wait until just before sleeping.
	 */
	void registerTimerCallback();

	TimerHandle_t _touchActivityTimer;
	TimerHandle_t _taskActivityTimer;
};

#endif