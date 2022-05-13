
#include <M5EPD.h>
#include <epdgui.h>

/* #include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "WebServer.h"
#include "secrets.h" */

enum
{
	BUTTON_SETTINGS,
	BUTTON_15MIN,
	BUTTON_30MIN,
	BUTTON_60MIN,
	BUTTON_90MIN,
	BUTTON_TILLNEXT,
	BUTTON_CONFIRM,
	BUTTON_CANCEL,
	BUTTON_FREEROOM
};

enum
{
	LABEL_CLOCK_UP,
	LABEL_BATTERY,
	LABEL_WIFI,
	LABEL_CLOCK_MID,
	LABEL_RESOURCE,
	LABEL_CURRENT_BOOKING,
	LABEL_BOOK_EVENT,
	LABEL_NEXT_EVENT,
	LABEL_NEXT_EVENT_CREATOR,
	LABEL_NEXT_EVENT_DESC,
	LABEL_NEXT_EVENT_TIME,
	LABEL_CURRENT_EVENT_CREATOR,
	LABEL_CURRENT_EVENT_DESC,
	LABEL_CURRENT_EVENT_TIME,
	LABEL_SIZE
};

struct Event {
	String startTime;
	String endTime;
	String creator;
	String description;
};

#define USE_HEADER_FONTS 0

#if USE_HEADER_FONTS
#include <Inter-Bold.ttf.h>
#include <Inter-Regular.ttf.h>

std::vector<std::pair<const unsigned char*, uint32_t>> fonts =
{{Inter_Bold_ttf, sizeof(Inter_Bold_ttf)},
{Inter_Regular_ttf, sizeof(Inter_Regular_ttf)}}

#else
std::vector<String> fonts = {"/lib/fonts/Inter-Regular.ttf", "/lib/fonts/Inter-Bold.ttf"};
#endif


#define TIME_FONT_SIZE = 44;
#define DEFAULT_FONT_SIZE = 24;

EPDGUI_Button *btns[9];
EPDGUI_Textbox *lbls[LABEL_SIZE];
M5EPD_Canvas canvasCurrentEvent(&M5.EPD);
M5EPD_Canvas canvasNextEvent(&M5.EPD);



/* const IPAddress SETUP_IP_ADDR(192, 168, 69, 1);
const char* SETUP_SSID = "BOOKING_SETUP";

// Initialize HTTP Server
WebServer webServer(80);

// Config store
Preferences preferences;

String WIFI_SSID = "";
String WIFI_PASS = "";

const char* NTP_SERVER = "time.google.com";
const char* TZ_INFO = "EET-2EEST,M3.5.0/3,M10.5.0/4";

bool restoreWifiConfig();
void setupMode();
String makePage(String title, String contents);

struct tm timeinfo;

void connectWifi(const char* ssid, const char* pass) {
	Serial.print(F("Connecting WiFi..."));

	WiFi.begin(ssid, pass);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
}

void setupTime() {
	Serial.println("Setting up time");

	// See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
	// for Timezone codes for your region
	configTzTime(TZ_INFO, NTP_SERVER);
}

void printLocalTime() {
	if (!getLocalTime(&timeinfo)) {
		Serial.println("Failed to obtain time 1");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
} */

void updateScreen() {
	canvasCurrentEvent.pushCanvas(0, 0, UPDATE_MODE_DU4);
	canvasNextEvent.pushCanvas(652, 0, UPDATE_MODE_DU4);
	
}

void settingsButton(epdgui_args_vector_t &args) {
	
	
}

void fifteenButton(epdgui_args_vector_t &args) {
	
}

void thirtyButton(epdgui_args_vector_t &args) {
	lbls[LABEL_NEXT_EVENT_TIME]->SetHide(true);

}

void sixtyButton(epdgui_args_vector_t &args) {
	lbls[LABEL_NEXT_EVENT_TIME]->SetHide(false);
}

void ninetyButton(epdgui_args_vector_t &args) {
	lbls[LABEL_NEXT_EVENT_TIME]->AddText("aaäÄöÖ");
}

void tillNextButton(epdgui_args_vector_t &args) {
	lbls[LABEL_NEXT_EVENT_TIME]->SetText("asdf");
	//updateScreen();
}

void confirmButton(epdgui_args_vector_t &args) {

}

void cancelButton(epdgui_args_vector_t &args) {

}

void freeRoomButton(epdgui_args_vector_t &args) {
}

void createButtons() {
	// Options button
	btns[BUTTON_SETTINGS] = new EPDGUI_Button("o", 0, 0, 64, 64);
	EPDGUI_AddObject(btns[BUTTON_SETTINGS]);
	btns[BUTTON_SETTINGS]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_SETTINGS]);
	btns[BUTTON_SETTINGS]->Bind(EPDGUI_Button::EVENT_RELEASED, settingsButton);

	// 15 min booking button
	btns[BUTTON_15MIN] = new EPDGUI_Button("15", 80, 306, 135, 77);
	EPDGUI_AddObject(btns[BUTTON_15MIN]);
	btns[BUTTON_15MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_15MIN]);
	btns[BUTTON_15MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, fifteenButton);
	
	// 30 min booking button
	btns[BUTTON_30MIN] = new EPDGUI_Button("30", 223, 306, 135, 77);
	EPDGUI_AddObject(btns[BUTTON_30MIN]);
	btns[BUTTON_30MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_30MIN]);
	btns[BUTTON_30MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, thirtyButton);

	// 60 min booking button
	btns[BUTTON_60MIN] = new EPDGUI_Button("60", 371, 306, 135, 77);
	EPDGUI_AddObject(btns[BUTTON_60MIN]);
	btns[BUTTON_60MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_60MIN]);
	btns[BUTTON_60MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, sixtyButton);

	// 90 min booking button
	btns[BUTTON_90MIN] = new EPDGUI_Button("90", 80, 399, 135, 77);
	EPDGUI_AddObject(btns[BUTTON_90MIN]);
	btns[BUTTON_90MIN]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_90MIN]);
	btns[BUTTON_90MIN]->Bind(EPDGUI_Button::EVENT_RELEASED, ninetyButton);

	// book till next event button
	btns[BUTTON_TILLNEXT] = new EPDGUI_Button("TILLNEXT", 223, 399, 365, 77);
	EPDGUI_AddObject(btns[BUTTON_TILLNEXT]);
	btns[BUTTON_TILLNEXT]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_TILLNEXT]);
	btns[BUTTON_TILLNEXT]->Bind(EPDGUI_Button::EVENT_RELEASED, tillNextButton);

	// confirm button
	btns[BUTTON_CONFIRM] = new EPDGUI_Button("Co", 300, 0, 100, 100);
	EPDGUI_AddObject(btns[BUTTON_CONFIRM]);
	btns[BUTTON_CONFIRM]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CONFIRM]);
	btns[BUTTON_CONFIRM]->Bind(EPDGUI_Button::EVENT_RELEASED, confirmButton);
	btns[BUTTON_CONFIRM]->SetHide(true);

	// cancel button
	btns[BUTTON_CANCEL] = new EPDGUI_Button("Ca", 450, 0, 100, 100);
	EPDGUI_AddObject(btns[BUTTON_CANCEL]);
	btns[BUTTON_CANCEL]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_CANCEL]);
	btns[BUTTON_CANCEL]->Bind(EPDGUI_Button::EVENT_RELEASED, settingsButton);
	btns[BUTTON_CANCEL]->SetHide(true);

	// free room button
	btns[BUTTON_FREEROOM] = new EPDGUI_Button("Free", 300, 100, 100, 100);
	EPDGUI_AddObject(btns[BUTTON_FREEROOM]);
	btns[BUTTON_FREEROOM]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, btns[BUTTON_FREEROOM]);
	btns[BUTTON_FREEROOM]->Bind(EPDGUI_Button::EVENT_RELEASED, freeRoomButton);
	btns[BUTTON_FREEROOM]->SetHide(true);
}

void createLabels() {
	Serial.println("Creating text labels");

	// upper right clock label
	lbls[LABEL_CLOCK_UP] = new EPDGUI_Textbox(885, 16, 64, 40, 3, 15);
	EPDGUI_AddObject(lbls[0]);
	lbls[LABEL_CLOCK_UP]->SetTextSize(24);
	lbls[LABEL_CLOCK_UP]->AddText("13");

	
	// battery status label
	lbls[LABEL_BATTERY] = new EPDGUI_Textbox(826, 16, 51, 40, 3, 15);
	EPDGUI_AddObject(lbls[LABEL_BATTERY]);
	lbls[LABEL_BATTERY]->SetTextSize(24);
	lbls[LABEL_BATTERY]->AddText("44");

	// wifi status label
	lbls[LABEL_WIFI] = new EPDGUI_Textbox(752, 16, 51, 40, 3, 15);
	EPDGUI_AddObject(lbls[LABEL_WIFI]);
	lbls[LABEL_WIFI]->SetTextSize(24);
	lbls[LABEL_WIFI]->AddText("OK");

	// middle clock label
	lbls[LABEL_CLOCK_MID] = new EPDGUI_Textbox(80, 92, 64, 40, 0, 15);
	EPDGUI_AddObject(lbls[LABEL_CLOCK_MID]);
	lbls[LABEL_CLOCK_MID]->SetTextSize(24);
	lbls[LABEL_CLOCK_MID]->AddText("44% ");
	// 498
	// resource label
	lbls[LABEL_RESOURCE] = new EPDGUI_Textbox(80, 125, 418, 40, 0, 15);
	EPDGUI_AddObject(lbls[LABEL_RESOURCE]);
	lbls[LABEL_RESOURCE]->SetTextSize(24);
	lbls[LABEL_RESOURCE]->AddText("Pieni neukkari 2");

	// current booking status label
	lbls[LABEL_CURRENT_BOOKING] = new EPDGUI_Textbox(80, 158, 418, 77, 0, 15);
	lbls[LABEL_CURRENT_BOOKING]->SetTextSize(60);
	lbls[LABEL_CURRENT_BOOKING]->AddText("Vapaa");
	EPDGUI_AddObject(lbls[LABEL_CURRENT_BOOKING]);

	// book event label
	lbls[LABEL_BOOK_EVENT] = new EPDGUI_Textbox(80, 251, 197, 39, 0, 15);
	lbls[LABEL_BOOK_EVENT]->SetTextSize(36);
	lbls[LABEL_BOOK_EVENT]->AddText("Varaa huone");
	EPDGUI_AddObject(lbls[LABEL_BOOK_EVENT]);

	// next event label
	lbls[LABEL_NEXT_EVENT] = new EPDGUI_Textbox(701, 161, 231, 78, 3, 15);
	lbls[LABEL_NEXT_EVENT]->SetTextSize(36);
	lbls[LABEL_NEXT_EVENT]->AddText("Seuraava\nvaraus");
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT]);

	// next event creator label
	lbls[LABEL_NEXT_EVENT_CREATOR] = new EPDGUI_Textbox(701, 246, 239, 40, 3, 15);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_CREATOR]);
	lbls[LABEL_NEXT_EVENT_CREATOR]->SetTextSize(24);
	lbls[LABEL_NEXT_EVENT_CREATOR]->AddText("Kalle");

	// next event desc label
	// here height is n*27 where n is the number of rows
	lbls[LABEL_NEXT_EVENT_DESC] = new EPDGUI_Textbox(701, 279, 231, 87, 3, 15);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_DESC]);
	lbls[LABEL_NEXT_EVENT_DESC]->SetTextSize(24);
	lbls[LABEL_NEXT_EVENT_DESC]->AddText("Videopalaveri\nasiakkaan kannsa\njatkuu");

	// next event time label
	lbls[LABEL_NEXT_EVENT_TIME] = new EPDGUI_Textbox(701, 370, 231, 106, 3, 15);
	EPDGUI_AddObject(lbls[LABEL_NEXT_EVENT_TIME]);
	lbls[LABEL_NEXT_EVENT_TIME]->SetTextSize(48);
	lbls[LABEL_NEXT_EVENT_TIME]->AddText("18.00 -\n21.00");

	// current event creator label
	lbls[LABEL_CURRENT_EVENT_CREATOR] = new EPDGUI_Textbox(80, 264, 412, 40, 15, 0);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_CREATOR]);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetTextSize(24);
	lbls[LABEL_CURRENT_EVENT_CREATOR]->AddText("Kalle Kallenen");
	lbls[LABEL_CURRENT_EVENT_CREATOR]->SetHide(true);

	// current event desc label
	lbls[LABEL_CURRENT_EVENT_DESC] = new EPDGUI_Textbox(80, 297, 412, 40, 15, 0);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_DESC]);
	lbls[LABEL_CURRENT_EVENT_DESC]->SetTextSize(24);
	lbls[LABEL_CURRENT_EVENT_DESC]->AddText("Videopalaveri");
	lbls[LABEL_CURRENT_EVENT_DESC]->SetHide(true);

	// current event time label
	lbls[LABEL_CURRENT_EVENT_TIME] = new EPDGUI_Textbox(80, 330, 412, 53, 15, 0);
	EPDGUI_AddObject(lbls[LABEL_CURRENT_EVENT_TIME]);
	lbls[LABEL_CURRENT_EVENT_TIME]->SetTextSize(36);
	lbls[LABEL_CURRENT_EVENT_TIME]->AddText("18:00 - 21:00");
	lbls[LABEL_CURRENT_EVENT_TIME]->SetHide(true);
	Serial.println("Done!");
	
}

void setup() {
	M5.begin();

	/* Serial.println(F("========== Monad Booking =========="));
	Serial.println(F("Booting up..."));

	Serial.println(F("Setting up E-ink display...")); */

    M5.EPD.SetRotation(0);
    M5.EPD.Clear(true);
    M5.RTC.begin();

	/* canvasCurrentEvent.createCanvas(652, 540);
	canvasCurrentEvent.fillCanvas(0);

	canvasNextEvent.createCanvas(308, 540);
	canvasNextEvent.fillCanvas(3); */

	Event testCurrentEvent;
	Event testNextEvent;

	testCurrentEvent.creator = "Kalle Kallenen";
	testCurrentEvent.description = "Videopalaveri";
	testCurrentEvent.startTime = "13:00";
	testCurrentEvent.endTime = "16.00";

	testNextEvent.creator = "Kalle Kallenen 2";
	testNextEvent.description = "Videopalaveri jatkuu";
	testNextEvent.startTime = "18:00";
	testNextEvent.endTime = "21.00";

	createButtons();
	createLabels();
	//updateScreen();












	/* Serial.println(F("Setting up RTC..."));
	M5.RTC.begin(); */

/* #ifdef USE_WEB_SETUP
	Serial.println(F("Restoring WiFi-configuration..."));

	if (restoreWifiConfig()) {
		Serial.println(F("WiFi-config restored!"));
		// connect
	} else {
		Serial.println(F("No wifi configuration stored"));
		Serial.println(F("Entering setup-mode..."));
		// Setupmode
		setupMode();
	}
	setRoutes();
	webServer.begin();
#else
	connectWifi(SECRETS_WIFI_SSID, SECRETS_WIFI_PASS);

	setupTime();
#endif */
}

/* bool restoreWifiConfig() {
	preferences.begin("wifi-config");
	delay(10);

	WIFI_SSID = preferences.getString("WIFI_SSID");
	WIFI_PASS = preferences.getString("WIFI_PASS");

	return WIFI_SSID.length() > 0;
}

String makePage(String title, String contents) {
	String s = "<!DOCTYPE html><html><head>";
	s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
	s += "<title>";
	s += title;
	s += "</title></head><body>";
	s += contents;
	s += "</body></html>";
	return s;
}

void setRoutes() {
	webServer.on("/", []() {
		Serial.println("Handling request for /");
		webServer.send(200, "text/html", makePage("TEST", "<h1>Toimiiko?</h1>"));
	});

	webServer.onNotFound([]() {
		String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
		Serial.println("Not found");
		webServer.send(200, "text/html", makePage("AP mode", s));
	});
}

void setupMode() {
	Serial.printf("Starting Access Point at \"%s\"\n", SETUP_SSID);
	WiFi.disconnect();
	delay(100);
	WiFi.softAPConfig(SETUP_IP_ADDR, SETUP_IP_ADDR, IPAddress(255, 255, 255, 0));
	WiFi.softAP(SETUP_SSID);
	WiFi.mode(WIFI_MODE_AP);
} */

void loop() {
/* #ifdef USE_WEB_SETUP
	webServer.handleClient();
#endif
	printLocalTime();
	delay(1000); */
	EPDGUI_Run();
}
