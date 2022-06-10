#include "timeUtils.h"

namespace timeutils {
time_t getNextMidnight(time_t now) {
	tmElements_t tm;
	ezt::breakTime(now, tm);

	tm.Hour = 0;
	tm.Minute = 0;
	tm.Second = 0;
	tm.Day += 1;

	return ezt::makeTime(tm);
}

time_t parseRfcTimestamp(const String& input) {
	tmElements_t tmElements{};
	tmElements.Year = input.substring(0, 4).toInt() - 1970;
	tmElements.Month = input.substring(5, 7).toInt();
	tmElements.Day = input.substring(8, 10).toInt();
	tmElements.Hour = input.substring(11, 13).toInt();
	tmElements.Minute = input.substring(14, 16).toInt();
	tmElements.Second = input.substring(17, 19).toInt();

	time_t t = ezt::makeTime(tmElements);

	// Find position of timezone, because we need to ignore fractions of seconds
	unsigned int timeZonePos = 19;
	if (input.charAt(19) == '.') {
		timeZonePos = input.indexOf("Z", 19);
		if (timeZonePos == -1)
			timeZonePos = input.indexOf("+", 19);
		if (timeZonePos == -1)
			timeZonePos = input.indexOf("-", 19);
	}

	// Timezone handling
	if (input.charAt(timeZonePos) != 'Z') {
		int sign = 1;
		if (input.charAt(timeZonePos) == '-') {
			sign = -1;
		}

		int offsetHours = input.substring(timeZonePos + 1, timeZonePos + 3).toInt();
		int offsetMinutes = input.substring(timeZonePos + 4, timeZonePos + 6).toInt();

		t -= sign * offsetHours * SECS_PER_HOUR;
		t -= sign * offsetMinutes * SECS_PER_MIN;
	}

	return t;
}
}