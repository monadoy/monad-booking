#include "timeUtils.h"

namespace timeutils {

RTCDateTime toRTCTime(time_t unixTime) {
	tmElements_t tm;
	ezt::breakTime(unixTime, tm);

	// Conversions:
	// Year:
	// - ezTime: offset since 1970, rtc: year numbers normally
	// Month:
	// - ezTime: 1 to 12, rtc: same
	// Day:
	// - ezTime: 1 to 31, rtc: same
	// Weekday:
	// - ezTime: 1 to 7 starting Sunday, rtc: 0 to 6 starting Sunday
	// Hour:
	// - ezTime: 0 to 23, rtc: same
	// Minute:
	// - ezTime: 0 to 59, rtc: same
	// Second:
	// - ezTime: 0 to 59, rtc: same

	RTC_Date date{static_cast<int8_t>(tm.Wday - 1), static_cast<int8_t>(tm.Month),
	              static_cast<int8_t>(tm.Day), static_cast<int16_t>(tm.Year + 1970)};
	RTC_Time time{static_cast<int8_t>(tm.Hour), static_cast<int8_t>(tm.Minute),
	              static_cast<int8_t>(tm.Second)};

	return RTCDateTime{.date = std::move(date), .time = std::move(time)};
}

time_t toUnixTime(RTCDateTime rtcDateTime) {
	const RTC_Date& date = rtcDateTime.date;
	const RTC_Time& time = rtcDateTime.time;

	tmElements_t tm{
	    .Second = static_cast<uint8_t>(time.sec),
	    .Minute = static_cast<uint8_t>(time.min),
	    .Hour = static_cast<uint8_t>(time.hour),
	    .Wday = static_cast<uint8_t>(date.week + 1),
	    .Day = static_cast<uint8_t>(date.day),
	    .Month = static_cast<uint8_t>(date.mon),
	    .Year = static_cast<uint8_t>(date.year - 1970),
	};

	return ezt::makeTime(tm);
}

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
}  // namespace timeutils