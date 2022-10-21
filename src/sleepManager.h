#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ezTime.h>

#include <atomic>
#include <mutex>
#include <vector>

#define SLEEP_MANAGER_MAX_TASK_COUNT 20

#define SLEEP_MANAGER_TASK_STACK_SIZE (1024 * 3)
#define SLEEP_MANAGER_TASK_PRIORITY 10
#define SLEEP_MANAGER_TASK_QUEUE_SIZE 5

// How long to keep awake after touch event
#define TOUCH_WAKE_TIMEOUT_MS (10 * 1000)

// How long the keep awake after tasks are complete.
// This works as a safety buffer when there is some processing time between
// two sequential tasks that should keep us awake.
#define TASK_WAKE_TIMEOUT_MS 1000

// Minimum time to put into timer when sleeping
#define MIN_TIMED_SLEEP_S 3

#define STATUS_UPDATE_INTERVAL_S (2 * SECS_PER_MIN)

#define WAKEUP_SAFETY_BUFFER_S 10

#define ERROR_REBOOT_DELAY_S (20 * SECS_PER_MIN)

class SleepManager;

class ScopedTaskCounter {
  public:
	ScopedTaskCounter(SleepManager* manager);
	~ScopedTaskCounter();

  private:
	SleepManager* _manager;
};

/**
 * Handles going to sleep and waking up from touch or a reccurring timer.
 * Makes the device sleep when there are no active tasks and there hasn't been a touch input in some
 * time. Members starting with underlines indicate pseudo private members, don't use them. (FreeRTOS
 * task API needs them public to work)
 */
class SleepManager {
  public:
	SleepManager();

	enum class Action : size_t {
		SLEEP,         // Go to sleep until `nextWakeTime`
		SHUTDOWN,      // Shut down and wake tomorrow based on `_onHours and _onMinutes`
		ERROR_REBOOT,  // Shut down and reboot in ERROR_REBOOT_DELAY_S
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

	/**
	 * Asks sleepmanager to shut down as soon as tasks have completed.
	 * Wake up after ERROR_REBOOT_DELAY_S to retry.
	 */
	void requestErrorReboot();

	enum class Callback : size_t {
		/*
		 * Is called on both touch and timer wake up
		 * and happens before them.
		 */
		AFTER_WAKE,
		AFTER_WAKE_TOUCH,
		AFTER_WAKE_TIMER,
		BEFORE_SLEEP,
		BEFORE_SHUTDOWN,
		SIZE
	};

	static const std::array<const char*, (size_t)Callback::SIZE> callbackNames;

	// Set this to control length of time to sleep, if it's less than current time, sleep will be
	// default length. Uses unix utc seconds.
	std::atomic<time_t> nextWakeTime{0};

	void registerCallback(Callback type, const std::function<void()>& cb);
	std::mutex _callbacksMutex;  // Protects _callbacks
	std::array<std::vector<std::function<void()>>, (size_t)Callback::SIZE> _callbacks{};
	void _dispatchCallbacks(Callback type);

	void setOnTimes(JsonObjectConst config);
	std::mutex _onTimesMutex;  // Protects onDays, onHours and onMinutes
	std::array<bool, 7> _onDays{1, 1, 1, 1, 1, 1, 1};
	std::array<uint8_t, 2> _onHours{0, 0};
	std::array<uint8_t, 2> _onMinutes{0, 0};

	time_t calculateTurnOnTimeUTC(time_t localNow);
	bool _shouldShutdown();

	/**
	 * Set wakeAfter to -1 to wake up tomorrow based on _onHours and _onMinutes.
	 */
	void _shutdown(time_t wakeAfter);

	void _error_reboot();

	enum class WakeReason { TOUCH, TIMER, UNKNOWN };
	WakeReason _sleep();

	void _enqueue(Action action);

	bool _anyActivity();

	SemaphoreHandle_t _activeTaskCounter;

	TimerHandle_t _touchActivityTimer;
	TimerHandle_t _taskActivityTimer;

	QueueHandle_t _queueHandle;
	TaskHandle_t _taskHandle;
};

#endif