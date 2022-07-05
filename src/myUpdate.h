#ifndef MY_UPDATE_H
#define MY_UPDATE_H

#include <Arduino.h>

#include <array>

#include "utils.h"

std::array<int, 3> getAvailableFirmwareVersion();

String versionToString(std::array<int, 3> version);

bool isNewer(std::array<int, 3> newVersion);

std::unique_ptr<utils::Error> updateFirmware(std::array<int, 3> newVersion,
                                             std::function<void()> onBeforeFormat);

#endif