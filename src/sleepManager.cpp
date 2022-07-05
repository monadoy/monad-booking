#include "sleepManager.h"

#include "esp_wifi.h"
#include "globals.h"
#include "timeUtils.h"
#include "utils.h"

ScopedTaskCounter::ScopedTaskCounter(SleepManager* manager) : _manager{manager} {
	_manager->incrementTaskCounter();
}
ScopedTaskCounter::~ScopedTaskCounter() { _manager->decrementTaskCounter(); }

typedef SleepManager SM;

const std::array<const char*, (size_t)SM::Callback::SIZE> SM::callbackNames{
    "AFTER_WAKE", "AFTER_WAKE_TOUCH", "AFTER_WAKE_TIMER", "BEFORE_SLEEP", "BEFORE_SHUTDOWN",
};

void handleBeforeAction(SM* manager, SM::Action action) {
	switch (action) {
		case SM::Action::SLEEP:
			manager->_dispatchCallbacks(SM::Callback::BEFORE_SLEEP);
			break;
		case SM::Action::SHUTDOWN:
			manager->_dispatchCallbacks(SM::Callback::BEFORE_SHUTDOWN);
			break;
		default:
			log_e("unhandled action %d", SM::callbackNames[(size_t)action]);
	}
}

void handleAction(SM* manager, SM::Action action) {
	switch (action) {
		case SM::Action::SLEEP: {
			const SM::WakeReason wakeReason = manager->_sleep();

			manager->_dispatchCallbacks(SM::Callback::AFTER_WAKE);
			if (wakeReason == SM::WakeReason::TIMER)
				manager->_dispatchCallbacks(SM::Callback::AFTER_WAKE_TIMER);
			else if (wakeReason == SM::WakeReason::TOUCH)
				manager->_dispatchCallbacks(SM::Callback::AFTER_WAKE_TOUCH);
			break;
		}
		case SM::Action::SHUTDOWN:
			manager->_shutdown();
			break;
		default:
			log_e("unhandled action %d", SM::callbackNames[(size_t)action]);
	}
}

void task(void* arg) {
	SM* manager = static_cast<SM*>(arg);

	log_i("Task created");

	for (;;) {
		SM::Action action;
		xQueueReceive(manager->_queueHandle, &action, portMAX_DELAY);

		handleBeforeAction(manager, action);

		// BEFORE_* callbacks may spin up tasks that keep us awake.
		// Wait until they are done.
		// TODO: could use semaphores instead of polling
		delay(10);
		log_i("Waiting for BEFORE_* tasks to complete");
		while (manager->_anyActivity()) {
			delay(10);
		}
		log_i("Waiting done!");

		// Remove all new queue actions added by callback tasks
		xQueueReset(manager->_queueHandle);

		handleAction(manager, action);
	}

	vTaskDelete(NULL);
}

// Timer callbacks can't block in any way
void timerCallback(TimerHandle_t handle) {
	SleepManager* manager = static_cast<SleepManager*>(pvTimerGetTimerID(handle));

	if (manager->_anyActivity())
		return;

	log_i("All timers and tasks expired");

	if (manager->_shouldShutdown()) {
		manager->_enqueue(SleepManager::Action::SHUTDOWN);
	} else {
		manager->_enqueue(SleepManager::Action::SLEEP);
	}
}

bool SleepManager::_anyActivity() {
	return xTimerIsTimerActive(_taskActivityTimer) == pdTRUE
	       || xTimerIsTimerActive(_touchActivityTimer) == pdTRUE
	       || uxSemaphoreGetCount(_activeTaskCounter) > 0;
}

void SleepManager::_enqueue(Action action) { xQueueSend(_queueHandle, (void*)&action, 0); }

SleepManager::SleepManager() {
	xTaskCreate(task, "SleepManager Task", SLEEP_MANAGER_TASK_STACK_SIZE, static_cast<void*>(this),
	            SLEEP_MANAGER_TASK_PRIORITY, &_taskHandle);

	_queueHandle = xQueueCreate(SLEEP_MANAGER_TASK_QUEUE_SIZE, sizeof(Action));
	assert(_queueHandle != NULL);

	_activeTaskCounter = xSemaphoreCreateCounting(SLEEP_MANAGER_MAX_TASK_COUNT, 0);
	assert(_activeTaskCounter != NULL);

	_touchActivityTimer = xTimerCreate("TOUCH_ACTIVITY", pdMS_TO_TICKS(TOUCH_WAKE_TIMEOUT_MS),
	                                   pdFALSE, (void*)this, timerCallback);
	assert(_touchActivityTimer != NULL);

	_taskActivityTimer = xTimerCreate("TASK_ACTIVITY", pdMS_TO_TICKS(TASK_WAKE_TIMEOUT_MS), pdFALSE,
	                                  (void*)this, timerCallback);
	assert(_taskActivityTimer != NULL);
}

ScopedTaskCounter SleepManager::scopedTaskCount() { return ScopedTaskCounter{this}; }

void SleepManager::incrementTaskCounter() {
	if (xSemaphoreGive(_activeTaskCounter) != pdTRUE) {
		log_e("Task counter increment failed");
		return;
	}

	// log_i("Task counter incremented: count %d", uxSemaphoreGetCount(_activeTaskCounter));
}

void SleepManager::decrementTaskCounter() {
	if (xSemaphoreTake(_activeTaskCounter, 10) != pdTRUE) {
		log_e("Task counter decrement failed");
		return;
	}

	BaseType_t count = uxSemaphoreGetCount(_activeTaskCounter);
	// log_i("Task counter decremented: count %d", count);

	if (count == 0) {
		if (xTimerReset(_taskActivityTimer, 10) != pdPASS) {
			log_e("Activity activity timer reset failed");
		}
	}
}

void SleepManager::refreshTouchWake() {
	if (xTimerReset(_touchActivityTimer, 10) != pdPASS) {
		log_e("Touch activity timer reset failed");
	}

	// log_i("Touch wake refreshed");
}

void SleepManager::registerCallback(Callback type, const std::function<void()>& cb) {
	std::lock_guard<std::mutex> lock(_callbacksMutex);
	_callbacks[(size_t)type].push_back(cb);
}

void SleepManager::_dispatchCallbacks(Callback type) {
	log_i("Dispatching callback %s.", SM::callbackNames[(size_t)type]);
	std::lock_guard<std::mutex> lock(_callbacksMutex);
	for (const auto& cb : _callbacks[(size_t)type]) cb();
}

SleepManager::WakeReason SleepManager::_sleep() {
	log_i("Going to sleep...");

	uint64_t sleepTime = max(nextWakeTime - safeUTC.now(), (long)MIN_TIMED_SLEEP_S);

	log_i("Sleeping for %llu s or until touch.", sleepTime);

	Serial.flush();

	wifiManager.sleepWiFi();

	// Light sleep and wait for timer or touch interrupt
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);  // TOUCH_INT
	esp_sleep_enable_timer_wakeup(sleepTime * 1000 * 1000);
	esp_light_sleep_start();

	wifiManager.wakeWiFi();

	auto cause = esp_sleep_get_wakeup_cause();
	if (cause == ESP_SLEEP_WAKEUP_TIMER) {
		log_i("Wakeup cause: TIMER");
		return WakeReason::TIMER;
	} else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
		log_i("Wakeup cause: TOUCH");
		return WakeReason::TOUCH;
	}

	return WakeReason::UNKNOWN;
}

void SleepManager::requestShutdown() { _enqueue(Action::SHUTDOWN); }

void SleepManager::setOnTimes(const std::array<bool, 7>& days, const std::array<uint8_t, 2>& hours,
                              const std::array<uint8_t, 2>& minutes) {
	std::lock_guard<std::mutex> lock(_onTimesMutex);
	_onDays = days;
	_onHours = hours;
	_onMinutes = minutes;
	log_i("Awake days: %d %d %d %d %d %d %d.", _onDays[0], _onDays[1], _onDays[2], _onDays[3],
	      _onDays[4], _onDays[5], _onDays[6]);
	log_i("Awake times: from: %02d:%02d, to: %02d:%02d.", _onHours[0], _onMinutes[0], _onHours[1],
	      _onMinutes[1]);
}

time_t SleepManager::calculateTurnOnTimeUTC(time_t localNow) {
	std::lock_guard<std::mutex> lock(_onTimesMutex);

	tmElements_t tm;
	ezt::breakTime(localNow, tm);
	if (tm.Hour > _onHours[0] || (tm.Hour == _onHours[0] && tm.Minute >= _onMinutes[0])) {
		tm.Day += 1;
	}
	tm.Hour = _onHours[0];
	tm.Minute = _onMinutes[0] + 1;  // Add one minute buffer in case timer drifts
	tm.Second = 0;

	return safeMyTZ.tzTime(ezt::makeTime(tm));
}

bool SleepManager::_shouldShutdown() {
	std::lock_guard<std::mutex> lock(_onTimesMutex);

	time_t t = safeMyTZ.now();
	tmElements_t tm;
	ezt::breakTime(t, tm);

	return (!_onDays[tm.Wday - 1]
	        || (tm.Hour < _onHours[0] || (tm.Hour == _onHours[0] && tm.Minute < _onMinutes[0]))
	        || (tm.Hour > _onHours[1] || (tm.Hour == _onHours[1] && tm.Minute > _onMinutes[1]))
	               && !utils::isCharging());
}

void SleepManager::_shutdown() {
	time_t now = safeMyTZ.now();
	time_t turnOnTime = calculateTurnOnTimeUTC(now);

	timeutils::RTCDateTime turnOnTimeRTC = timeutils::toRTCTime(turnOnTime);

	String turnOnTimeStr = safeMyTZ.dateTime(turnOnTime, UTC_TIME, RFC3339);

	const String log
	    = "[" + safeMyTZ.dateTime(now, RFC3339) + "] Shut down, try wake at " + turnOnTimeStr;
	log_i("%s", log.c_str());
	utils::addBootLogEntry(log);

	Serial.flush();

	M5.shutdown(turnOnTimeRTC.date, turnOnTimeRTC.time);
}
