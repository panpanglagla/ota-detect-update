#include <Arduino.h>
#include <SPI.h>
#include <HTTPClient.h>

#include "constants.h"
#include "tm.h"
#include "network.h"

Preferences appSettings;
Network network;
TimeManager timeManager;

StateType state = StateType_Start;

boolean networkConnected = false;
boolean dateSynced = false;
unsigned long t;
unsigned long lastCheck = 0;

long unsigned CHECK_INTERVAL_IN_SECONDS = 30;

void setup() {
  Serial.begin(115200);
  delay(1000);

  network.setup();
  appSettings.begin("ota-detect-update");
}

void loop() {

  if (networkConnected) {
    //Serial.println("Network connected");
    network.handleClient();
  }

  switch (state) {
    case StateType_Start:
      if (!dateSynced) {
        if (!networkConnected) {
          state = StateType_StartNetwork;
          return;
        } else {
          state = StateType_TimeSync;
          return;
        }
      } else {
      }
      return;
    case StateType_StartNetwork:
      if (!networkConnected) {
        networkConnected = network.connect();
        if (!networkConnected) {
          state = StateType_StartHotspot;
          return;
        } else {
          state = StateType_TimeSync;
          return;
        }
      }
      return;
    case StateType_StartHotspot:
      if (!networkConnected) {
        networkConnected = network.startAP();
        if (!networkConnected) {
          state = StateType_Error;
          return;
        } else {
          state = StateType_Hotspot;
          return;
        }
      } else {
        state = StateType_Hotspot;
      }
      return;
    case StateType_Hotspot:
      return;
    case StateType_TimeSync:
      if (!dateSynced) {
        dateSynced = timeManager.sync();
        if (dateSynced) {
          state = StateType_Ready;
          return;
        } else {
          state = StateType_Error;
          return;
        }
      }
      return;
    case StateType_Error:
      return;
    case StateType_Ready:
      lastCheck = appSettings.getLong("lastcheck", 0);
      state = StateType_Idle;
      return;
    case StateType_Idle:
      blink("red", 1, 500);
      t = (long)timeManager.getNowUTC();
      Serial.print("Next check in ");
      Serial.print(CHECK_INTERVAL_IN_SECONDS - t + lastCheck);
      Serial.println(" seconds");
      if (lastCheck == 0 || t - lastCheck > CHECK_INTERVAL_IN_SECONDS) {
        state = StateType_Check;
      }
      return;
    case StateType_Check:
      Serial.println("Checking available versions");
      otaCheck();
      t = (long)timeManager.getNowUTC();
      appSettings.putLong("lastcheck", t);
      lastCheck = t;
      Serial.print("lastCheck: ");
      Serial.println(lastCheck);
      state = StateType_Idle;
      return;
  }
}

void blink(String color, int times, long wait) {
  for( int i = 0 ; i < times; i++ ) {

    digitalWrite(LED_BLUE, color == "blue" ? LOW : HIGH);
    digitalWrite(LED_GREEN, color == "green" ? LOW : HIGH);
    digitalWrite(LED_RED, color == "red" ? LOW : HIGH);
    digitalWrite(LED_BUILTIN, HIGH);

    delay(wait);

    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);

    delay(wait);
  }
}

void otaCheck() {

  HTTPClient http;

  Serial.println("otaCheck");
  http.begin("https://raw.githubusercontent.com/panpanglagla/ota-detect-update/main/versions/list.json");

  Serial.println("[HTTP] GET...\n");

  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("payload");
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
