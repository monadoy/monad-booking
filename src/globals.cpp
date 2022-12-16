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
    = String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH)
      + (STRINGIZE(VERSION_PRERELEASE).length() > 0 ? "-" + STRINGIZE(VERSION_PRERELEASE) : "")
         + (STRINGIZE(VERSION_BUILD).length() > 0 ? "+" + STRINGIZE(VERSION_BUILD) : "");
utils::Result<String> latestVersionResult
    = utils::Result<String>::makeErr(new utils::Error("Not initialized."));
