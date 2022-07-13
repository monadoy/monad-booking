#ifndef SAFE_TIMEZONE_H
#define SAFE_TIMEZONE_H

#include <ezTime.h>

#include <mutex>

/**
 * A thread safe version of ezTime Timezone class.
 * Only implements functions that are used by us.
 */
class SafeTimezone {
  public:
	SafeTimezone() : tz_(UTC) {}
	SafeTimezone(Timezone& tz) : tz_(tz) {}

	time_t now() {
		xSemaphoreTake(handle_, portMAX_DELAY);
		time_t t = tz_.now();
		xSemaphoreGive(handle_);
		return t;
	}

	String dateTime(const String format /* = DEFAULT_TIMEFORMAT */) {
		xSemaphoreTake(handle_, portMAX_DELAY);
		String s = dateTime(TIME_NOW, format);
		xSemaphoreGive(handle_);
		return s;
	}

	String dateTime(const time_t t, const String format /* = DEFAULT_TIMEFORMAT */) {
		return dateTime(t, LOCAL_TIME, format);
	}

	String dateTime(time_t t, const ezLocalOrUTC_t local_or_utc,
	                const String format /* = DEFAULT_TIMEFORMAT */) {
		return tz_.dateTime(t, local_or_utc, format);
	}

	time_t tzTime(time_t t /* = TIME_NOW */) { return tz_.tzTime(t); }

	String getOlson() { return tz_.getOlson(); }

	bool setCache(const String name, const String key) {
		xSemaphoreTake(handle_, portMAX_DELAY);
		bool res = tz_.setCache(name, key);
		xSemaphoreGive(handle_);
		return res;
	}

	bool setLocation(const String location /* = "GeoIP" */) {
		xSemaphoreTake(handle_, portMAX_DELAY);
		bool res = tz_.setLocation(location);
		xSemaphoreGive(handle_);
		return res;
	}

	void setTime(const time_t t, const uint16_t ms = 0) {
		xSemaphoreTake(handle_, portMAX_DELAY);
		tz_.setTime(t, ms);
		xSemaphoreGive(handle_);
	}

  private:
	SemaphoreHandle_t handle_ = xSemaphoreCreateMutex();
	Timezone& tz_;
};

#endif