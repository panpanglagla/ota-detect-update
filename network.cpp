#include "common.h"
#include "constants.h"
#include "network.h"

Network::Network()
  : _server(80) {
  _header = "";
}

void Network::setup() {
  WiFi.mode(WIFI_STA);
  restore();
}

boolean Network::connected() {
  return WiFi.status() == WL_CONNECTED;
}

boolean Network::connect() {
  _appSettings.begin("otadetectupdate", false);
  _versionSettings.begin("versionsettings", false);

  MDNS.end();
  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_STA);

  long numWifi = getWifiCount();
  if (numWifi == 0) {
    return false;
  }

  byte maxWait = 10;
  byte tick = 0;
  delay(500);

  for (_i = 0; _i < numWifi; _i++) {
    if (_names[_i] != "" && _passwords[_i] != "") {
      tick = 0;
      WiFi.begin(_names[_i], _passwords[_i]);
      while (tick < maxWait && !connected()) {
        delay(1000);
        tick++;
      }
    }
  }
  Serial.println("");

  if (connected()) {
    if (!MDNS.begin("ota-detect-update")) {
      return false;
    }

    MDNS.addService("http", "tcp", 80);
    
    _server.on("/", HTTP_GET, [=]() {
      _server.send(200, "text/html", renderHome());
    });

    _server.on("/js", HTTP_GET, [=]() {
      _server.send(200, "text/html", renderJS());
    });

    _server.on("/css", HTTP_GET, [=]() {
      _server.send(200, "text/html", _css.getCSS());
    });

    _server.on("/versions", HTTP_GET, [=]() {
      _server.send(200, "text/html", getVersions());
    });

    _server.on("/ota-check", HTTP_POST, [=]() {
      otaCheck();
      _server.send(200, "text/json", "{}");
    });

    _server.on("/install-version", HTTP_POST, [=]() {
      String json = _server.arg(0).c_str();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      const char* version = doc["version"];
      setNextVersion(version);
      _server.send(200, "text/json", "{}");
    });

    _server.on("/wifi", HTTP_DELETE, [=]() {
      deleteWifis();
      _server.send(200, "text/json", "{}");
    });

    _server.begin();
  } else {
    Serial.println(F("Not connected"));
  }
  return connected();
}

boolean Network::startAP() {
  MDNS.end();
  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_AP);
  delay(500);
  if (WiFi.softAP("ota-detect-update", "panpanglagla!1234")) {
    IPAddress myIP = WiFi.softAPIP();
  } else {
    return false;
  }

  delay(500);

  if (!WiFi.softAPConfig(IPAddress(192, 168, 1, 12), IPAddress(192, 168, 1, 12), IPAddress(255, 255, 255, 0))) {
    return false;
  }

  if (!MDNS.begin("ota-detect-update")) {
    return false;
  }

  MDNS.addService("http", "tcp", 80);

  _server.on("/", HTTP_GET, [=]() {
    _server.send(200, "text/html", renderWifiSettings());
  });

  _server.on("/wifi", HTTP_POST, [=]() {
    String json = _server.arg(0).c_str();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);
    const char* wifi = doc["wifi"];
    const char* pwd = doc["pwd"];
    Serial.print("wifi: ");
    Serial.print(wifi);
    Serial.print("| pwd: ");
    Serial.println(pwd);
    setWifi(wifi, pwd);
    _server.send(200, "text/json", "{}");
  });

  _server.on("/wifi", HTTP_DELETE, [=]() {
    deleteWifis();
    _server.send(200, "text/json", "{}");
  });

  _server.on("/js", HTTP_GET, [=]() {
    _server.send(200, "text/html", renderJS());
  });

  _server.on("/css", HTTP_GET, [=]() {
    _server.send(200, "text/html", _css.getCSS());
  });

  _server.begin();

  return true;
}

void Network::handleClient() {
  _server.handleClient();
}

// Private
String Network::renderJS() {
  String str = "";
  String nextVersion = _versionSettings.getString("nextVersion", "");
  str.concat("const nextVersion = \"");
  str.concat(nextVersion);
  str.concat("\";\n");
  str.concat("\n");
  str.concat("const appVersion = \"");
  str.concat(VERSION);
  str.concat("\";\n");
  str.concat("\n");
  str.concat("function makeElement(type, classes, inner) {\n");
  str.concat("  var elem = document.createElement(type);\n");
  str.concat("  var i;\n");
  str.concat("  if (classes && classes.length > 0) {\n");
  str.concat("    var classesArray = classes.split(\" \");\n");
  str.concat("    for (i = 0; i < classesArray.length; i++) {\n");
  str.concat("      classesArray[i] && elem.classList.add(classesArray[i]);\n");
  str.concat("    }\n");
  str.concat("  }\n");
  str.concat("  if (inner !== undefined && inner !== null) {;\n");
  str.concat("    elem.innerHTML = inner;\n");
  str.concat("  }\n");
  str.concat("  return elem;\n");
  str.concat("}\n");

  str.concat("function installAfterReboot(version) {\n");
  str.concat("  const xhr = new XMLHttpRequest();\n");
  str.concat("  xhr.open(\"POST\", '/install-version');\n");
  str.concat("  xhr.send(JSON.stringify({ version }));\n");
  str.concat("  xhr.onload = function(e) {\n");
  str.concat("    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {\n");
  str.concat("      const response = JSON.parse(xhr.responseText);\n");
  str.concat("      window.location.reload();\n");
  str.concat("    }\n");
  str.concat("  }\n");
  str.concat("}\n");

  str.concat("function installVersion(version) {\n");
  str.concat("  return function() { installAfterReboot(version); }\n");
  str.concat("}\n");

  str.concat("function getVersions() {\n");
  str.concat("  const fieldWifi = document.querySelector(\"#wifi\");\n");
  str.concat("  const fieldPwd = document.querySelector(\"#pwd\");\n");
  str.concat("  const xhr = new XMLHttpRequest();\n");
  str.concat("  xhr.open(\"GET\", '/versions');\n");
  str.concat("  xhr.send();\n");
  str.concat("  xhr.onload = function(e) {\n");
  str.concat("    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {\n");
  str.concat("      const container = document.querySelector(\"#container\");\n");
  str.concat("      const response = JSON.parse(xhr.responseText);\n");
  str.concat("      const versions = makeElement(\"div\", \"f g ai-s col\");\n");
  str.concat("      container.appendChild(versions);\n");
  str.concat("      for(var i = 0 ; i < response.length ; i++){\n");
  str.concat("        const version = makeElement(\"div\", \"f g ai-s ai-c \");\n");
  str.concat("        const current = response[i] == appVersion;\n");
  str.concat("        const next = response[i] == nextVersion;\n");
  str.concat("        const selectable = !(current || next);\n");
  str.concat("        const cssCommon = \"pad-l fs-xl white radius if border b\";\n");
  
  str.concat("        const cssPointer = selectable ? \"pointer\" : \"\";\n");
  str.concat("        const cssColor = current ? \"blue border-blue text-white\" : ( next ? \"green border-green text-white\" : \"white border-green\");\n");
  str.concat("        const versionTag = makeElement(\"button\", cssCommon + \" \" + cssPointer + \" \" + cssColor, response[i]);\n");
  str.concat("        if (selectable) { versionTag.onclick=installVersion(response[i]) } else { versionTag.disabled = true }\n");
  str.concat("        version.appendChild(versionTag);\n");
  str.concat("        if (appVersion == response[i]) {\n");
  str.concat("          version.appendChild(makeElement(\"span\", \"text-blue if b it\", \"Current version\"))\n");
  str.concat("        }\n");
  str.concat("        if (nextVersion == response[i]) {\n");
  str.concat("          const cancelDiv = makeElement(\"div\", \"if col g\")\n");
  str.concat("          cancelDiv.appendChild(makeElement(\"span\", \"text-green if b\", \"Version to install after next reboot\"))\n");
  str.concat("          const cancelButton = makeElement(\"button\", \"pointer radius green text-white border-green if b\", \"Cancel install\")\n");
  str.concat("          cancelButton.onclick=installVersion(\"\")\n");
  str.concat("          cancelDiv.appendChild(cancelButton)\n");
  str.concat("          version.appendChild(cancelDiv)\n");
  str.concat("        }\n");
  str.concat("        versions.appendChild(version);\n");
  str.concat("      }\n");
  str.concat("    }\n");
  str.concat("  }\n");
  str.concat("}\n");

  str.concat("function setWifi() {\n");
  str.concat("  const fieldWifi = document.querySelector(\"#wifi\");\n");
  str.concat("  const fieldPwd = document.querySelector(\"#pwd\");\n");
  str.concat("  const xhr = new XMLHttpRequest();\n");
  str.concat("  xhr.open(\"POST\", '/wifi');\n");
  str.concat("  xhr.send(JSON.stringify({ wifi: fieldWifi.value, pwd: fieldPwd.value }));\n");
  str.concat("  xhr.onload = function(e) {\n");
  str.concat("    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {\n");
  str.concat("      const response = JSON.parse(xhr.responseText);\n");
  str.concat("      window.location.reload();\n");
  str.concat("    }\n");
  str.concat("  }\n");
  str.concat("}\n");

  str.concat("function deleteWifis() {\n");
  str.concat("  if (!confirm(\"Delete wifi credentials ?\")) return;\n");
  str.concat("  const xhr = new XMLHttpRequest();\n");
  str.concat("  xhr.open(\"DELETE\", '/wifi');\n");
  str.concat("  xhr.send();\n");
  str.concat("  xhr.onload = function(e) {\n");
  str.concat("    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {\n");
  str.concat("      window.location.reload();\n");
  str.concat("    }\n");
  str.concat("  }\n");
  str.concat("}\n");

  str.concat("function otaCheck() {\n");
  str.concat("  const xhr = new XMLHttpRequest();\n");
  str.concat("  xhr.open(\"POST\", '/ota-check');\n");
  str.concat("  xhr.send();\n");
  str.concat("  xhr.onload = function(e) {\n");
  str.concat("    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {\n");
  str.concat("      window.location.reload();\n");
  str.concat("    }\n");
  str.concat("  }\n");
  str.concat("}\n");

  return str;
}

String Network::renderWifiSettings() {
  int num = getWifiCount();
  String str = "";
  str.concat("<html>\n");
  str.concat("  <head>\n");
  str.concat("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n");
  str.concat("    <title>WIFI settings</title>\n");
  str.concat("    <link rel=\"stylesheet\" href=\"css\" >\n");
  str.concat("    <script type=\"text/javascript\" src=\"js\"></script>\n");
  str.concat("  </head>\n");
  str.concat("  <body>\n");
  str.concat("    <label for=\"wifi\" class=\"text-blue b fs-l\">WIFI name</label>\n");
  str.concat("    <br />\n");
  str.concat("    <input id=\"wifi\" minlength=\"4\" maxlength=\"50\" class=\"radius pad border border-gray-d gray-l text-white fs-l\" />\n");
  str.concat("    <br />\n");
  str.concat("    <br />\n");
  str.concat("    <label for=\"pwd\" class=\"text-blue b fs-l mar-t\">WIFI passphrase</label>\n");
  str.concat("    <br />\n");
  str.concat("    <input id=\"pwd\" type=\"password\" minlength=\"8\" maxlength=\"60\" class=\"radius pad border border-gray-d gray-l text-white fs-l\" />\n");
  str.concat("    <br />\n");
  str.concat("    <br />\n");
  str.concat("    <button class=\"radius border border-gray-d green pointer fs-xl b white \" title=\"Save\" onclick=\"setWifi();\">\n");
  str.concat("      Add wifi");
  str.concat("    </button>\n");
  str.concat("    <br />\n");
  str.concat("    <br />\n");
  str.concat("    <hr />\n");
  str.concat("    <br />\n");
  if (num > 0) {
    str.concat("    <span class=\"text-gray-d\">\n");
    str.concat("Registered wifis:\n");
    str.concat("    </span>\n");
    str.concat("    <table>\n");
    for (_i = 0; _i < num; _i++) {
      str.concat("  <tr>\n");
      str.concat("    <td class=\"b text-gray-d fs-xs\">\n");
      str.concat("      <span>\n");
      str.concat(_names[_i]);
      str.concat("      </span>\n");
      str.concat("    </td>\n");
      str.concat("    <td class=\"b text-gray-d fs-xs\">\n");
      str.concat("      <input type=\"password\" value=\"");
      str.concat(_passwords[_i]);
      str.concat("\"/>\n");
      str.concat("    </td>\n");
      str.concat("  </tr>\n");
    }
    str.concat("</table>\n");

    str.concat("    <button class=\"radius border border-gray-d red pointer fs-xl b white\" title=\"Delete\" onclick=\"deleteWifis();\">\n");
    str.concat("      Delete all");
    str.concat("    </button>\n");
  } else {
    str.concat("    <span class=\"text-gray-d\">\n");
    str.concat("No registered wifi !\n");
    str.concat("    </span>\n");
  }

  str.concat("  </body>\n");
  str.concat("</html>\n");

  return str;
}

String Network::getVersions() {
  if (availableVersions != "") {
    return availableVersions;
  }
  return "[]";
}

String Network::renderHome() {
  String str = "";
  str.concat("<html>\n");
  str.concat("  <head>\n");
  str.concat("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n");
  str.concat("    <title>OTA Detect Update</title>\n");
  str.concat("    <link rel=\"stylesheet\" href=\"css\" >\n");
  str.concat("    <script type=\"text/javascript\" src=\"js\"></script>\n");
  str.concat("  </head>\n");
  str.concat("  <body>\n");
  str.concat("    <div class=\"mar f col g-xl ai-c\">\n");
  str.concat("        <button class=\"radius border border-red white pointer fs-xl b text-red\" title=\"Delete wifi credentials\" onclick=\"deleteWifis();\">\n");
  str.concat("          Delete wifi credentials");
  str.concat("        </button>\n");
  str.concat("        <button class=\"radius border border-blue blue pointer fs-xl b text-white\" title=\"Check for newers version now\" onclick=\"otaCheck();\">\n");
  str.concat("          Check for newers version now");
  str.concat("        </button>\n");
  str.concat("      <div id=\"container\" class=\"m-auto\" />\n");
  str.concat("    </div>\n");
  str.concat("    <script type=\"text/javascript\">\n");
  str.concat("      getVersions();\n");
  str.concat("    </script>\n");
  str.concat("  </body>\n");
  str.concat("</html>\n");

  return str;
}

void Network::restore() {
  _settings.begin("network", false);
  String prefWifi = _settings.getString("names", "");
  String prefPwd = _settings.getString("passwords", "");

  StringSplitter* splitterWifi = new StringSplitter(prefWifi, ',', MAX_WIFIS);
  StringSplitter* splitterPwd = new StringSplitter(prefPwd, ',', MAX_WIFIS);

  int countWifi = splitterWifi->getItemCount();
  int countPwd = splitterPwd->getItemCount();
  String valueWifi;
  String valuePwd;

  Serial.println(F("Restoring wifi..."));
  for (_i = 0; _i < MAX_WIFIS; _i++) {
    valueWifi = splitterWifi->getItemAtIndex(_i);
    valuePwd = splitterPwd->getItemAtIndex(_i);
    if (_i + 1 <= countWifi && valueWifi != "" && valuePwd != "") {
      _names[_i] = valueWifi;
      _passwords[_i] = valuePwd;
    }
  }
  save();
}

void Network::save() {
  boolean isFirst = true;
  String strWifi = "";
  String strPwd = "";
  for (_i = 0; _i < MAX_WIFIS; _i++) {
    isFirst = _i == 0;
    strWifi += isFirst ? "" : ",";
    strPwd += isFirst ? "" : ",";
    strWifi += _names[_i];
    strPwd += _passwords[_i];
  }
  _settings.putString("names", strWifi);
  _settings.putString("passwords", strPwd);
}

void Network::deleteWifis() {
  for (_i = 0; _i < MAX_WIFIS; _i++) {
    _names[_i] = "";
    _passwords[_i] = "";
  }
  save();
  restore();
}

void Network::otaCheck() {
 // _appSettings.putLong("lastcheck", 0);
}

void Network::setWifi(String wifi, String pwd) {
  long num = getWifiCount();
  Serial.println(num);

  if (wifi != "" && pwd != "") {
    boolean exists = false;
    for (int i = 0; i < num; i++) {
      if (_names[i] == wifi && _passwords[i] == pwd) {
        exists = true;
        break;
      }
    }
    if (exists) {
      return;
    }
  } else {
    return;
  }
  _names[num] = wifi;
  _passwords[num] = pwd;
  save();
  return;
}

void Network::setNextVersion(String nextVersion) {
  Serial.print("Set version to be installed after reboot: ");
  Serial.println(nextVersion);
  Serial.println(_versionSettings.getString("nextVersion", "?"));
  _versionSettings.putString("nextVersion", nextVersion);
  return;
}

long Network::getWifiCount() {
  long num = 0;
  for (int i = 0; i < MAX_WIFIS; i++) {
    if (_names[i] != "" && _passwords[i] != "") {
      num++;
    } else {
      break;
    }
  }
  return num;
}
