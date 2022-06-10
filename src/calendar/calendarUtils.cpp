#include "calendarUtils.h"

namespace calutils {
void printEvent(const cal::Event& event, SafeTimezone& tz) {
	Serial.println("EVENT: {");
	Serial.println("  Id: " + event.id);
	Serial.println("  Creator: " + event.creator);
	Serial.println("  Summary: " + event.summary);
	Serial.println("  Start Timestamp: " + tz.dateTime(event.unixStartTime, UTC_TIME, RFC3339));
	Serial.println("  End Timestamp: " + tz.dateTime(event.unixEndTime, UTC_TIME, RFC3339));
	Serial.println("}");
}
}  // namespace calutils