#include "globals.h"

Timezone _myTZ;
SafeTimezone safeMyTZ{_myTZ};
SafeTimezone safeUTC{UTC};
SleepManager sleepManager;
WiFiManager wifiManager;
Localization l10n;
Preferences preferences;

const Version CURRENT_VERSION{VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH};
utils::Result<Version> latestVersionResult
    = utils::Result<Version>::makeErr(new utils::Error("Not initialized."));