#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5EPD.h>

#include <memory>
#include <vector>

namespace utils {

struct Error {
	Error(const String& message) : message(message) {}
	String message;
};

/**
 * Contains either a successful result or an error.
 * Check first which value is contained with functions isOk() or isErr().
 * Then you can get a shared_ptr to the value with functions ok() or err().
 */
template <typename T, typename E = Error>
class Result {
  private:
	Result(std::shared_ptr<T> ok, std::shared_ptr<E> err) : _ok{ok}, _err{err} {}

	std::shared_ptr<T> _ok;
	std::shared_ptr<E> _err;

  public:
	static Result makeOk(T* value) {
		assert(!!value);
		return Result(std::shared_ptr<T>(value), nullptr);
	}

	static Result makeOk(std::shared_ptr<T> value) {
		assert(!!value);
		return Result(value, nullptr);
	}

	static Result makeErr(E* error) {
		assert(!!error);
		return Result(nullptr, std::shared_ptr<E>(error));
	}

	static Result makeErr(std::shared_ptr<E> error) {
		assert(!!error);
		return Result(nullptr, error);
	}

	bool isOk() const { return _ok != nullptr; }
	bool isErr() const { return _err != nullptr; }

	std::shared_ptr<T> ok() const {
		assert(isOk());
		return _ok;
	}

	std::shared_ptr<E> err() const {
		assert(isErr());
		return _err;
	}
};

float getBatteryLevel();

bool isCharging();

std::vector<String> getBootLog();

bool addBootLogEntry(const String& entry);

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

String httpCodeToString(int code);

void merge(JsonVariant dst, JsonVariantConst src);

void forceRestart();

std::vector<String> listFiles(const String& directory);

}  // namespace utils

#endif