#include "sleepManager.h"

#include "esp_wifi.h"
#include "globals.h"
#include "utils.h"

ScopedTaskCounter::ScopedTaskCounter(SleepManager* manager) : _manager{manager} {
	_manager->incrementTaskCounter();
}
ScopedTaskCounter::~ScopedTaskCounter() { _manager->decrementTaskCounter(); }

typedef SleepManager SM;

void task(void* arg) {
	SM* manager = static_cast<SM*>(arg);

	log_i("Task created");

	for (;;) {
		SM::Action action;
		xQueueReceive(manager->_queueHandle, &action, portMAX_DELAY);

		switch (action) {
			case SM::Action::SLEEP: {
				manager->_dispatchCallbacks(SM::Callback::BEFORE_SLEEP);

				// If any callback started a task, we should postpone sleeping
				if (manager->_anyActivity())
					continue;

				const SM::WakeReason wakeReason = manager->_sleep();

				manager->_dispatchCallbacks(SM::Callback::AFTER_WAKE);
				if (wakeReason == SM::WakeReason::TIMER)
					manager->_dispatchCallbacks(SM::Callback::AFTER_WAKE_TIMER);
				else if (wakeReason == SM::WakeReason::TOUCH)
					manager->_dispatchCallbacks(SM::Callback::AFTER_WAKE_TOUCH);

				break;
			}
			default:
				log_e("unhandled action %d", (size_t)action);
				break;
		}
	}

	vTaskDelete(NULL);
}

// Timer callbacks can't block in any way
void timerCallback(TimerHandle_t handle) {
	SleepManager* manager = static_cast<SleepManager*>(pvTimerGetTimerID(handle));

	if (manager->_anyActivity())
		return;

	log_i("All timers and tasks expired");

	manager->_enqueue(SleepManager::Action::SLEEP);
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
	if (_queueHandle == NULL) {
		Serial.println("SleepManager Task: queue create failed");
	}

	_activeTaskCounter = xSemaphoreCreateCounting(SLEEP_MANAGER_MAX_TASK_COUNT, 0);
	if (_activeTaskCounter == NULL) {
		log_e("Active task counter semaphore creation failed");
	}

	_touchActivityTimer = xTimerCreate("TOUCH_ACTIVITY", pdMS_TO_TICKS(TOUCH_WAKE_TIMEOUT_MS),
	                                   pdFALSE, (void*)this, timerCallback);
	if (_touchActivityTimer == NULL) {
		log_e("Touch activity timer creation failed");
	}

	_taskActivityTimer = xTimerCreate("TASK_ACTIVITY", pdMS_TO_TICKS(TASK_WAKE_TIMEOUT_MS), pdFALSE,
	                                  (void*)this, timerCallback);
	if (_taskActivityTimer == NULL) {
		log_e("Task activity timer creation failed");
	}
}

ScopedTaskCounter SleepManager::scopedTaskCount() { return ScopedTaskCounter{this}; }

void SleepManager::incrementTaskCounter() {
	if (xSemaphoreGive(_activeTaskCounter) != pdTRUE) {
		log_e("Task counter increment failed");
		return;
	}
}

void SleepManager::decrementTaskCounter() {
	if (xSemaphoreTake(_activeTaskCounter, 10) != pdTRUE) {
		log_e("Task counter decrement failed");
		return;
	}

	BaseType_t count = uxSemaphoreGetCount(_activeTaskCounter);

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
	_callbacks[(size_t)type].push_back(cb);
}

void SleepManager::_dispatchCallbacks(Callback type) {
	for (const auto& cb : _callbacks[(size_t)type]) cb();
}

SleepManager::WakeReason SleepManager::_sleep() {
	log_i("Going to sleep...");

	time_t now = safeUTC.now();

	uint64_t sleepTime = max(nextWakeTime - now, (long)MIN_TIMED_SLEEP_S);

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