#include "utils.h"

#include <LittleFS.h>

namespace utils {
bool isCharging() { return M5.getBatteryVoltage() > 4275; }

std::vector<String> getBootLog() {
	fs::File readHandle = LittleFS.open("/boot.log", FILE_READ);
	if (!readHandle) {
		return std::vector<String>();
	}

	std::vector<String> entries{};
	String line;
	while (true) {
		line = readHandle.readStringUntil('\n');
		if (line.isEmpty())
			break;
		entries.push_back(std::move(line));
	}
	readHandle.close();

	return entries;
}

bool addBootLogEntry(const String& entry) {
	auto entries = getBootLog();

	if (entries.size() > 11) {
		entries.erase(entries.begin());
	}

	entries.push_back(entry);

	fs::File writeHandle = LittleFS.open("/boot.log", FILE_WRITE, true);
	if (!writeHandle) {
		return false;
	}

	for (const String& entry : entries) {
		writeHandle.println(entry);
	}
	writeHandle.close();
	return true;
}

}  // namespace utils