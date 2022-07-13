#include "globals.h"

Timezone _myTZ;
SafeTimezone safeMyTZ{_myTZ};
SafeTimezone safeUTC{UTC};
SleepManager sleepManager;
WiFiManager wifiManager;
Localization l10n;
Preferences preferences;

const String CURRENT_VERSION_STRING
    = String(VERSION_MAJOR) + "." + String(VERSION_MINOR) + "." + String(VERSION_PATCH);

std::array<int, 3> latestVersion = {0, 0, 0};