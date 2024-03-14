#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <Arduino.h>

const String appVersion = "APP_VERSION";
const String fbqn = "DEVICE_FBQN";
const String binariesFolder = "BINARIES_FOLDER";

typedef enum {
  StateType_Start,
  StateType_StartNetwork,
  StateType_StartHotspot,
  StateType_Hotspot,
  StateType_TimeSync,
  StateType_Error,
  StateType_Ready,
  StateType_Idle,
  StateType_Check
} StateType;

#endif
