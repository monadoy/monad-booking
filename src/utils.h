#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
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
struct Result {
  private:
	Result(std::shared_ptr<T> _ok, std::shared_ptr<E> _err) : ok_{_ok}, err_{_err} {}

	const std::shared_ptr<T> ok_ = nullptr;
	const std::shared_ptr<E> err_ = nullptr;

  public:
	Result(const Result& other) = default;
	Result(Result&& other) = default;
	Result& operator=(const Result& other) = default;
	Result& operator=(Result&& other) = default;
	virtual ~Result() = default;

	static Result makeOk(T* value) {
		if (!value)
			throw std::runtime_error("Tried to make an ok result with a null pointer");
		return Result(std::shared_ptr<T>(value), nullptr);
	}

	static Result makeOk(std::shared_ptr<T> value) {
		if (!value)
			throw std::runtime_error("Tried to make an ok result with a null pointer");
		return Result(value, nullptr);
	}

	static Result makeErr(E* error) {
		if (!error)
			throw std::runtime_error("Tried to make an error result with a null pointer");
		return Result(nullptr, std::shared_ptr<E>(error));
	}

	static Result makeErr(std::shared_ptr<E> error) {
		if (!error)
			throw std::runtime_error("Tried to make an error result with a null pointer");
		return Result(nullptr, error);
	}

	bool isOk() const { return ok_ != nullptr; }
	bool isErr() const { return err_ != nullptr; }

	std::shared_ptr<T> ok() const {
		assert(isOk());
		return ok_;
	}

	std::shared_ptr<E> err() const {
		assert(isErr());
		return err_;
	}
};

void connectWiFi(const String& ssid, const String& password);

void ensureWiFi();

void sleepWiFi();

bool isCharging();

bool isAP();

String getApPassword();

String getApSSID();

bool isSetupMode();

void setupMode();

String genRandomAppendix(int length);

std::vector<String> getBootLog();

bool addBootLogEntry(const String& entry);

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace utils

#endif