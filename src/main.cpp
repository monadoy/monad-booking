
#include <M5EPD.h>
#include <WiFi.h>

#include "secrets.h"

M5EPD_Canvas canvas(&M5.EPD);

struct tm timeinfo;

void setupTime() {
	Serial.println("Setting up time");

	M5.RTC.begin();

	configTime(0, 0, NTP_SERVER);
	if (!getLocalTime(&timeinfo)) {
		Serial.println("Failed to obtain time");
		return;
	}
	// See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
	// for Timezone codes for your region
	setenv("TZ", TZ_INFO, 1);
	tzset();
}

void setupWifi() {
	Serial.println("Setting up wifi");

	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
}

void printLocalTime() {
	if (!getLocalTime(&timeinfo)) {
		Serial.println("Failed to obtain time 1");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void setup() {
	M5.begin();
	M5.EPD.SetRotation(0);
	M5.EPD.Clear(true);

	setupWifi();
	setupTime();

	// canvas.createCanvas(540, 960);
	canvas.createCanvas(960, 540);
	canvas.setTextSize(3);
	canvas.fillCanvas(0xeeef);
	canvas.drawString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 110, 110);
	// canvas.drawJpgUrl("https://m5stack.oss-cn-shenzhen.aliyuncs.com/image/example_pic/flower.jpg");
	// canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
	// canvas.createCanvas(540, 960);
	// canvas.setTextSize(3);
	// canvas.drawJpgUrl("https://m5stack.oss-cn-shenzhen.aliyuncs.com/image/example_pic/flower.jpg");
	canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
	Serial.print(canvas.readPixel(100, 100));
}

void loop() {
	printLocalTime();
	delay(1000);
}