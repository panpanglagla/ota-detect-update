#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <Arduino.h>

typedef enum {
  StateType_Start,
  StateType_StartNetwork,
  StateType_StartHotspot,
  StateType_Hotspot,
  StateType_TimeSync,
  StateType_InstallFirmware,
  StateType_Error,
  StateType_Ready,
  StateType_Idle,
  StateType_Versions,
  StateType_Reboot
} StateType;

const String VERSION = "APP_VERSION";
const String FQBN = "DEVICE_FQBN";
const String BIN_FOLDER = "BINARIES_FOLDER";

const int FIRMWARE_UPGRADE_MAX_ATTEMPTS = 5;

const long unsigned CHECK_INTERVAL_IN_SECONDS = 120;

const String BIN_FILE = "ota-detect-update.ino.bin";
const String SERVER = "raw.githubusercontent.com";
const String COMMON_PATH = "/panpanglagla/ota-detect-update/main";
const String LIST_FILE = "/versions/list.json";
const String VERSIONS_PATH = "/versions";

#endif
