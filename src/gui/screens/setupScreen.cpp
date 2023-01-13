#include "setupScreen.h"

#include "globals.h"
#include "gui/displayUtils.h"

namespace gui {

SetupScreen::SetupScreen() {
	const int txt_pad = 60;
	const int txt_w = 960 - 2 * txt_pad;

	ADD_PNL(PNL_MAIN, Panel(Pos{0, 0}, Size{960, 540}, WH));

	ADD_TXT(TXT_TITLE,
	        Text(Pos{txt_pad, txt_pad}, Size{txt_w, 77}, "Setup", FS_TITLE, BK, WH, true));
	ADD_TXT(TXT_MAIN,
	        Text(Pos{txt_pad, txt_pad + 77 + 20}, Size{txt_w, 540 - txt_pad - 77 - 20 - txt_pad},
	             "Setup", FS_NORMAL, BK, WH));

	ASSERT_ALL_ELEMENTS();
}

void SetupScreen::startSetup(bool useAP) {
	// Increment and never decrement in order to never got to sleep
	sleepManager.incrementTaskCounter();

	_configStore = utils::make_unique<config::ConfigStore>(LittleFS);
	_configServer = utils::make_unique<config::ConfigServer>(80, _configStore.get());

	if (useAP) {
		wifiManager.openAccessPoint();
		WiFiInfo info = wifiManager.getAccessPointInfo();
		_texts[TXT_MAIN]->setText("WIFI SSID: " + info.ssid + "\nPASSWORD: " + info.password
		                          + "\nIP: " + info.ip.toString());
	} else {
		wifiManager.waitWiFi();
		WiFiInfo info = wifiManager.getStationInfo();
		_texts[TXT_MAIN]->setText("WIFI SSID: " + info.ssid + "\nIP: " + info.ip.toString());
	}

	_configServer->start();
}

void drawQRCode(M5EPD_Canvas& c) {
	if (!wifiManager.isAccessPoint())
		return;

	M5EPD_Canvas canvasQR(&M5.EPD);
	canvasQR.createCanvas(360, 360);
	WiFiInfo info = wifiManager.getAccessPointInfo();
	const String qrString = "WIFI:S:" + info.ssid + ";T:WPA;P:" + info.password + ";;";
	canvasQR.qrcode(qrString, 0, 0, 360, 7);
	canvasQR.pushToCanvas(510, 90, &c);
}

void SetupScreen::draw(m5epd_update_mode_t mode) {
	M5EPD_Canvas& c = getScreenBuffer();
	for (auto& p : _panels) p->drawToCanvas(c);
	for (auto& t : _texts) t->drawToCanvas(c);
	for (auto& b : _buttons) b->drawToCanvas(c);
	drawQRCode(c);
	wakeDisplay();
	c.pushCanvas(0, 0, mode);
	sleepDisplay();
}

}  // namespace gui