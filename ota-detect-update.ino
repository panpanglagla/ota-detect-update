#include <Arduino.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <HttpsOTAUpdate.h>

#include "common.h"
#include "constants.h"
#include "tm.h"
#include "network.h"
#include "root_ca.h"

Preferences appSettings;
Preferences versionSettings;
Network network;
TimeManager timeManager;

StateType state = StateType_Start;

boolean networkConnected = false;
boolean dateSynced = false;
boolean startedFirmwareInstall = false;
unsigned long currentTime;
unsigned long lastVersionCheckTime = 0;
unsigned long disconnectedTime = 0;
String nextVersion = "";
int firmwareUpgradeAttempts = 0;

static HttpsOTAStatus_t otaStatus;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  printApplication();

  network.setup();
  appSettings.begin("otadetectupdate", false);
  versionSettings.begin("versionsettings", false);
}

void loop() {

  if (networkConnected) {
    // Serial.println("Network connected - Handle Client");
    network.handleClient();
  }

  switch (state) {
    case StateType_Start:
      return start();
    case StateType_StartNetwork:
      return startNetwork();
    case StateType_StartHotspot:
      return startHotspot();
    case StateType_Hotspot:
      return hotSpot();
    case StateType_TimeSync:
      return timeSync();
    case StateType_InstallFirmware:
      return installFirmware();
    case StateType_Error:
      return error();
    case StateType_Ready:
      return ready();
    case StateType_Idle:
      return idle();
    case StateType_Versions:
      return versions();
    case StateType_Reboot:
      return reboot();
  }
}

void start() {
  blink("blue", 3, 500);
  delay(2000);
  if (!dateSynced) {
    if (!networkConnected) {
      state = StateType_StartNetwork;
    } else {
      state = StateType_TimeSync;
    }
  }
  return;
}

void startNetwork() {
  if (!networkConnected) {
    networkConnected = network.connect();
    if (!networkConnected) {
      state = StateType_StartHotspot;
    } else {
      if (mustReinstallFirmware()) {
        state = StateType_InstallFirmware;
      } else {
        state = StateType_TimeSync;
      }
    }
  }
  return;
}

void startHotspot() {
  if (!networkConnected) {
    networkConnected = network.startAP();
    if (!networkConnected) {
      Serial.println("Started hotspot error");
      state = StateType_Error;
    } else {
      Serial.println("Started hotspot yeahhh");
      state = StateType_Hotspot;
    }
  } else {
    state = StateType_Hotspot;
  }
  return;
}

void hotSpot() {
  blink("green", 3, 500);
  delay(1000);
  return;
}

void timeSync() {
  blink("green", 3, 500);
  delay(1000);
  if (!dateSynced) {
    dateSynced = timeManager.sync();
    if (dateSynced) {
      state = StateType_Ready;
    } else {
      state = StateType_Error;
    }
  }
  return;
}

void error() {
  blink("red", 5, 500);
  delay(5000);
  return;
}

void ready() {
  blink("green", 1, 500);
  blink("red", 1, 500);
  blink("blue", 1, 500);
  delay(2000);
  lastVersionCheckTime = appSettings.getLong("lastcheck", 0);
  state = StateType_Idle;
  return;
}

void idle() {
  blink("green", 1, 200);
  blink("red", 2, 200);
  blink("blue", 3, 200);
  currentTime = (long)timeManager.getNowUTC();
  Serial.print("Next versions check in ");
  Serial.print(CHECK_INTERVAL_IN_SECONDS - currentTime + lastVersionCheckTime);
  Serial.println(" seconds");

  if (lastVersionCheckTime == 0 || currentTime - lastVersionCheckTime > CHECK_INTERVAL_IN_SECONDS) {
    state = StateType_Versions;
  }
  return;
}

void versions() {
  if (checkAvailableVersions()) {
    lastVersionCheckTime = (long)timeManager.getNowUTC();
    appSettings.putLong("lastcheck", lastVersionCheckTime);
  } else {
    state = StateType_Error;
  };
  state = StateType_Idle;
  return;
}

void installFirmware() {
  if (networkConnected && !startedFirmwareInstall && nextVersion != "") {
    startedFirmwareInstall = true;

    Serial.print("Installing version ");
    Serial.print(nextVersion);
    Serial.print(" [");
    Serial.print(firmwareUpgradeAttempts + 1);
    Serial.print("/");
    Serial.print(FIRMWARE_UPGRADE_MAX_ATTEMPTS);
    Serial.println("]");


    String bin = "https://";
    bin.concat(SERVER);
    bin.concat(COMMON_PATH);
    bin.concat(VERSIONS_PATH);
    bin.concat("/");
    bin.concat(nextVersion);
    bin.concat("/");
    bin.concat(BIN_FOLDER == "BINARIES_FOLDER" ? "" : BIN_FOLDER);
    bin.concat("/");
    bin.concat(BIN_FILE);

    HttpsOTA.onHttpEvent(HttpOTAEvent);

    Serial.print("Downloading firmware binary: ");
    Serial.println(bin);

    HttpsOTA.begin(bin.c_str(), root_ca, false);
    delay(1000);
  }
  if (startedFirmwareInstall) {
    otaStatus = HttpsOTA.status();

    if (otaStatus == HTTPS_OTA_SUCCESS) {
      Serial.println("Firmware written successfully");
      state = StateType_Reboot;
    } else if (otaStatus == HTTPS_OTA_FAIL) {
      if (firmwareUpgradeAttempts < FIRMWARE_UPGRADE_MAX_ATTEMPTS - 1) {
        startedFirmwareInstall = false;
        firmwareUpgradeAttempts++;
        delay(2000);
        Serial.println("Firmware upgrade: retry");
      } else {
        versionSettings.putString("nextVersion", "");
        nextVersion = "";
        Serial.println("Firmware upgrade failed");
        state = StateType_Error;
      }
      return;
    }
  }
}

void reboot() {
  versionSettings.putString("nextVersion", "");
  Serial.println("Restart device in 5 seconds");
  delay(5000);
  ESP.restart();
}

boolean mustReinstallFirmware() {
  nextVersion = versionSettings.getString("nextVersion", "");
  if (nextVersion == "") {
    Serial.println("No firware update expected");
    return false;
  } else {
    Serial.print("Install firmware ");
    Serial.println(nextVersion);
    return true;
  }
}

boolean checkAvailableVersions() {
  Serial.println("Getting available application versions...");

  HTTPClient http;

  String url = "https://";
  url.concat(SERVER);
  url.concat(COMMON_PATH);
  url.concat(LIST_FILE);

  http.begin(url);

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      availableVersions = payload;
      Serial.print("Application versions: ");
      Serial.println(availableVersions);
      http.end();
      return true;
    } else {
      Serial.print("Unexpected HTTP response getting application versions: ");
      Serial.println(http.errorToString(httpCode).c_str());
      http.end();
      return false;
    }
  } else {
    Serial.print("HTTP error getting application versions: ");
    Serial.println(http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

void HttpOTAEvent(HttpEvent_t *event) {
  Serial.print("HTTP OTA Event: ");
  switch (event->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("Error");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("Connected");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("Header sent");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.print("Header[");
      Serial.print(event->header_key);
      Serial.print("]: ");
      Serial.println(event->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println("Data received");
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("Finish");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("Disconnected");
      break;
  }
}

void printApplication() {
  Serial.print("Application version: ");
  Serial.println(VERSION);
  Serial.print("FQBN: ");
  Serial.println(FQBN);
  Serial.print("Binaries folder: ");
  Serial.println(BIN_FOLDER);
}

void blink(String color, int times, long wait) {
  for (int i = 0; i < times; i++) {

    digitalWrite(LED_BLUE, color == "blue" ? LOW : HIGH);
    digitalWrite(LED_GREEN, color == "green" ? LOW : HIGH);
    digitalWrite(LED_RED, color == "red" ? LOW : HIGH);
    digitalWrite(LED_BUILTIN, HIGH);

    delay(wait);

    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    delay(wait);
  }
}
