#include "globals.h"

Timezone _myTZ;
SafeTimezone safeMyTZ{_myTZ};
SafeTimezone safeUTC{UTC};
SleepManager sleepManager;
WiFiManager wifiManager;
Localization l10n;
Preferences preferences;
PNG png;

#define _STRINGIZE(x) #x
#define STRINGIZE(x) String(_STRINGIZE(x))

const String CURRENT_VERSION
    = String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH);

String UPDATE_CHANNEL = "stable";
utils::Result<String> latestVersionResult
    = utils::Result<String>::makeErr(new utils::Error("Not initialized."));
