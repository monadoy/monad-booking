#include "sleepManager.h"

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

	bool taskActive = xTimerIsTimerActive(manager->_taskActivityTimer) == pdTRUE;
	bool touchActive = xTimerIsTimerActive(manager->_touchActivityTimer) == pdTRUE;

	log_i("A timer expired, Task timer active: %d, Touch timer active: %d", taskActive,
	      touchActive);

	// If a timer is till active we don't need to do anything
	if (taskActive || touchActive) {
		return;
	}

	BaseType_t count = uxSemaphoreGetCount(manager->_activeTaskCounter);

	log_i("All timers expired, count: %d", count);

	// If active task counter is still zero after timer has expired,
	// go to sleep.
	if (count == 0) {
		manager->_enqueue(SleepManager::Action::SLEEP);
	}
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

	log_i("Task counter incremented, count: %d", uxSemaphoreGetCount(_activeTaskCounter));
}

void SleepManager::decrementTaskCounter() {
	if (xSemaphoreTake(_activeTaskCounter, 10) != pdTRUE) {
		log_e("Task counter decrement failed");
		return;
	}

	BaseType_t count = uxSemaphoreGetCount(_activeTaskCounter);
	log_i("Task counter decremented, count: %d", count);

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

	log_i("Touch wake refreshed");
}

void SleepManager::registerCallback(Callback type, const std::function<void()>& cb) {
	_callbacks[(size_t)type].push_back(cb);
}

void SleepManager::_dispatchCallbacks(Callback type) {
	for (const auto& cb : _callbacks[(size_t)type]) cb();
}

SleepManager::WakeReason SleepManager::_sleep() {
	log_i("Going to sleep... (NOT IMPLEMENTED YET)");

	// TODO: implement sleeping

	// // Calculate how long we need to sleep in order to achieve the desired update status
	// interval. unsigned long curMillis = millis(); uint64_t sleepTime
	//     = uint64_t(max(nextStatusUpdateMillis, curMillis + 1) - curMillis) * MICROS_PER_MILLI;

	// Serial.print("Sleeping for ");
	// Serial.print(sleepTime / MICROS_PER_SEC);
	// Serial.println(" s or until touch.");

	// Serial.flush();

	// utils::sleepWiFi();

	// // Light sleep and wait for timer or touch interrupt
	// esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);  // TOUCH_INT
	// esp_sleep_enable_timer_wakeup(sleepTime);
	// esp_light_sleep_start();

	// Serial.print("Wakeup cause: ");
	// auto cause = esp_sleep_get_wakeup_cause();
	// if (cause == ESP_SLEEP_WAKEUP_TIMER) {
	// 	Serial.println("TIMER");
	// 	Serial.println("Updating gui status and going back to sleep");

	// 	// When wakeup is caused by timer, a status update is due.
	// 	// gui::updateGui(); deprecated

	// 	// Calculate the next status update timestamp based on our interval.
	// 	nextStatusUpdateMillis = millis() + UPDATE_STATUS_INTERVAL_MS;

	// 	// Go instantly back to sleep
	// 	wakeUntilMillis = 0;
	// } else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
	// 	Serial.println("TOUCH");

	// 	// Stay awake for a moment to process long touches
	// 	wakeUntilMillis = millis() + WAKE_TIME_MS;
	// }

	return WakeReason::TIMER;
}