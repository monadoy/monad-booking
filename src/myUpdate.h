#ifndef MY_UPDATE_H
#define MY_UPDATE_H

#include <Arduino.h>

#include <array>

#include "utils.h"

const char* const CURR_BOOT_SUCCESS_KEY = "curr-b-ok";
const char* const LAST_BOOT_SUCCESS_KEY = "last-b-fail";
const char* const UPDATE_ON_NEXT_BOOT_KEY = "up-next";

void restart();

std::array<int, 3> getAvailableFirmwareVersion();

String versionToString(std::array<int, 3> version);

bool isVersionDifferent(std::array<int, 3> newVersion);

std::unique_ptr<utils::Error> updateFirmware(std::array<int, 3> newVersion,
                                             std::function<void()> onBeforeFormat);

#endif