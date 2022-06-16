#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H
#include <Arduino.h>
#include <ezTime.h>

#include <vector>

#define SLEEP_MANAGER_MAX_TASK_COUNT 20

#define SLEEP_MANAGER_TASK_STACK_SIZE 1024 * 4
#define SLEEP_MANAGER_TASK_PRIORITY 10
#define SLEEP_MANAGER_TASK_QUEUE_SIZE 10

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
 * Members starting with underlines indicate pseudo private members because tasks can't access truly
 * private members.
 */
class SleepManager {
  public:
	SleepManager();

	enum class Action : size_t {
		/*
		 * Timer callback is executed every TIMER_CALLBACK_INTERVAL_S. Device will
		 * wake up from sleep to ensure that the callback is executed in a timely manner. If already
		 * woken, we will wait until just before sleeping.
		 */
		INTERVAL,
		SLEEP,
		SIZE
	};

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
	 * Refresh touch wake timer to prevent going to sleep while user is using the GUI.
	 */
	void refreshTouchWake();

	enum class Callback : size_t {
		/*
		 * Is called on both touch and timer wake up
		 * and happens before them.
		 */
		AFTER_WAKE,
		AFTER_WAKE_TOUCH,
		AFTER_WAKE_TIMER,
		BEFORE_SLEEP,
		ON_INTERVAL,
		SIZE
	};

	/*
	 * WARNING!
	 * Register these before incrementing any task counters,
	 * otherwise there will be race conditions!!
	 */
	void registerCallback(Callback type, const std::function<void()>& cb);
	std::array<std::vector<std::function<void()>>, (size_t)Callback::SIZE> _callbacks{};
	void _dispatchCallbacks(Callback type);

	// TODO: callbacks for waking to touch and sleep
	// TODO: shutdown for the night if needed

	enum class WakeReason { TOUCH, TIMER };
	WakeReason _sleep();

	void _enqueue(Action action);

	SemaphoreHandle_t _activeTaskCounter;

	TimerHandle_t _touchActivityTimer;
	TimerHandle_t _taskActivityTimer;

	QueueHandle_t _queueHandle;
	TaskHandle_t _taskHandle;
};

#endif