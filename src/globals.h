#ifndef GLOBALS_H
#define GLOBALS_H

#include <PNGdec.h>
#include <Preferences.h>

#include "localization.h"
#include "myUpdate.h"
#include "safeTimezone.h"
#include "sleepManager.h"
#include "wifiManager.h"

// Pixel size of the largest png image
#define MAX_PNG_SIZE (376 * 248)
// We use 4 bits per pixel
#define PNG_BUFFER_SIZE (MAX_PNG_SIZE / 2)

const m5epd_update_mode_t MY_UPDATE_MODE = UPDATE_MODE_GC16;

extern SafeTimezone safeMyTZ;
extern SafeTimezone safeUTC;
extern SleepManager sleepManager;
extern WiFiManager wifiManager;
extern Localization l10n;
extern Preferences preferences;
extern PNG png;

extern String UPDATE_CHANNEL;
extern const String CURRENT_VERSION;
extern utils::Result<String> latestVersionResult;

#endif