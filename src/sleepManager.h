#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H
#include <Arduino.h>
#include <ezTime.h>

#define SLEEP_MANAGER_MAX_TASKS 10
#define INACTIVITY_TIMEOUT_S 15
#define WAKE_INTERVAL_S 2 * 60

class SleepManager;

class TaskCounterGuard {
  public:
	TaskCounterGuard(SleepManager* manager);
	~TaskCounterGuard();

  private:
	SleepManager* _manager;
};

class SleepManager {
  public:
	SleepManager();

	/**
	 * RAII lock guard for task counter.
	 * Increments when constructed, decrements when destructed.
	 * YOU NEED TO SAVE THE RESULT OF THIS FUNCTION IN A VARIABLE.
	 */
	[[nodiscard]] TaskCounterGuard createTaskGuard();

	/**
	 * You should probably be using createTaskGuard insted of these.
	 */
	void incrementTaskCounter();
	void decrementTaskCounter();

	// TODO: callbacks for waking to touch and sleep
	// TODO: shutdown for the night if needed

	void _sleep();
	SemaphoreHandle_t _activeTaskCounter;

  private:
	TimerHandle_t _activityTimeoutTimer;
	time_t lastWakeTime;
};

#endif