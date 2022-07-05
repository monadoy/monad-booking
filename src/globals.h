#ifndef GLOBALS_H
#define GLOBALS_H

#include "localization.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "wifiManager.h"

extern SafeTimezone safeMyTZ;
extern SafeTimezone safeUTC;
extern SleepManager sleepManager;
extern WiFiManager wifiManager;
extern Localization l10n;
extern const String CURRENT_VERSION_STRING;

#endif