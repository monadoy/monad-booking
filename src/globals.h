#ifndef GLOBALS_H
#define GLOBALS_H

#include <Preferences.h>

#include "localization.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "wifiManager.h"

extern SafeTimezone safeMyTZ;
extern SafeTimezone safeUTC;
extern SleepManager sleepManager;
extern WiFiManager wifiManager;
extern Localization l10n;
extern Preferences preferences;

extern const String CURRENT_VERSION_STRING;

#endif