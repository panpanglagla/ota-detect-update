#ifndef TM_H
#define TM_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Timezone.h>
#include "constants.h"

class TimeManager {
public:
  TimeManager();
  boolean sync();
  time_t getNowUTC();
};

#endif