#ifndef MY_UPDATE_H
#define MY_UPDATE_H

#include <Arduino.h>

#include <array>

#include "utils.h"

const char* const CURR_BOOT_SUCCESS_KEY = "curr-b-ok";
const char* const LAST_BOOT_SUCCESS_KEY = "last-b-ok";
const char* const MANUAL_UPDATE_KEY = "up-force";

struct Version {
  public:
	Version(int major, int minor, int patch) : major{major}, minor{minor}, patch{patch} {}

	String toString() const { return String(major) + "." + String(minor) + "." + String(patch); }
	bool operator==(const Version& other) const {
		return major == other.major && minor == other.minor && patch == other.patch;
	}

	bool operator!=(const Version& other) const { return !(*this == other); }

	int major = 0;
	int minor = 0;
	int patch = 0;
};

/**
 * Restarts the device, uses a workaround to force restart even when the device plugged in.
 */
void restart();

/**
 * Fetch information about the latest available firmware version from the internet.
 * Returns nullptr on failure.
 */
utils::Result<Version> getLatestFirmwareVersion();

/**
 * Fetches new firmware and filesystem from the internet based on newVersion.
 * onBeforeFilesystemWrite() callback is called before the filesystem is overwritten.
 * Use it to stop all reading and writing operations to avoid race conditions.
 */
std::unique_ptr<utils::Error> updateFirmware(const Version& newVersion,
                                             std::function<void()> onBeforeFilesystemWrite);

#endif