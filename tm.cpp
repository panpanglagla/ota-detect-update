#include "tm.h"

TimeChangeRule CEST = { "CEST", Last, Sun, Mar, 2, 120 };  // Central European Summer Time
TimeChangeRule CET = { "CET ", Last, Sun, Oct, 3, 60 };    // Central European Standard Time
Timezone CE(CEST, CET);

TimeManager::TimeManager() {
}

boolean TimeManager::sync() {
  WiFiUDP _ntpUDP;
  NTPClient _timeClient(_ntpUDP, "fr.pool.ntp.org", 0, 60 * 60 * 1000);
  _timeClient.begin();
  if (_timeClient.update()) {
    unsigned long epoch = _timeClient.getEpochTime();
    setTime(epoch);
    return true;
  } else {
    return false;
  }
}

time_t TimeManager::getNowUTC() {
  return now();
}
