#pragma once
#include <Preferences.h>
#include <Update.h>
#include "BuildIdentity.h"

// The existing per-device LocalNetworkConfig.h is migrated into NVS once.
// Later OTA builds use the saved settings and require no private credentials.
static Preferences cdNetworkPrefs;
static String cdSsid, cdPassword;
static bool cdSetupMode = false, cdOtaAccepted = false, cdOtaFailed = false;
static bool cdRestartPending = false;
static unsigned long cdRestartAt = 0;

static void cdConfigureNetwork() {
  cdNetworkPrefs.begin("cd-net", false);
  cdSsid = cdNetworkPrefs.getString("ssid", "");
  cdPassword = cdNetworkPrefs.getString("password", "");
  if (cdSsid.length() == 0 && !String(HOME_WIFI_SSID).startsWith("REPLACE_")) {
    cdSsid = HOME_WIFI_SSID;
    cdPassword = HOME_WIFI_PASSWORD;
    cdNetworkPrefs.putString("ssid", cdSsid);
    cdNetworkPrefs.putString("password", cdPassword);
  }
  cdSetupMode = (cdSsid.length() == 0);
  if (cdSetupMode) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("LiteOn-CD-Setup", "LiteOnCDsetup");
    Serial.println("[CD] Set up Wi-Fi at http://192.168.4.1/setup");
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cdSsid.c_str(), cdPassword.c_str());
  }
}

static void cdFirmwareInfo() {
  String info = "{\"version\":\"" + String(CD_RELEASE_VERSION) +
                "\",\"sourceSha\":\"" + String(CD_SOURCE_SHA) +
                "\",\"sketchMd5\":\"" + ESP.getSketchMD5() +
                "\",\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) +
                ",\"bootSession\":\"" + String(bootSession) + "\"}";
  ui.send(200, "application/json", info);
}

// OTA upload is allowed only from the configured House Music Pi.
// This is LAN source-IP restriction, not strong cryptographic authentication.
// The service must only be used on a trusted home network.
static bool cdRequestFromPi() {
  IPAddress expected;
  return !cdSetupMode && expected.fromString(gatewayIP) &&
         ui.client().remoteIP() == expected;
}

static void cdFirmwareUploadData() {
  if (!cdRequestFromPi()) { cdOtaFailed = true; return; }
  HTTPUpload &upload = ui.upload();
  if (upload.status == UPLOAD_FILE_START) {
    cdOtaFailed = false;
    cdOtaAccepted = false;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      cdOtaFailed = true;
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!cdOtaFailed && Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      cdOtaFailed = true;
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!cdOtaFailed && Update.end(true)) {
      cdOtaAccepted = true;
      Serial.printf("[CD OTA] Received %u bytes\n", upload.totalSize);
    } else {
      cdOtaFailed = true;
      Update.abort();
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    cdOtaFailed = true;
    Update.abort();
  }
}

static void cdFirmwareUploadFinish() {
  if (!cdRequestFromPi()) { ui.send(403, "text/plain", "Pi only"); return; }
  if (cdOtaFailed || !cdOtaAccepted) {
    ui.send(500, "text/plain", "OTA rejected"); return;
  }
  ui.send(200, "application/json", "{\"accepted\":true,\"rebooting\":true}");
  cdRestartPending = true;
  cdRestartAt = millis() + 1500;
}

static void cdSetupPage() {
  if (!cdSetupMode) { ui.send(404, "text/plain", "Not in setup mode"); return; }
  ui.send(200, "text/html",
    "<!doctype html><html><body><h2>LiteOn CD Wi-Fi setup</h2>"
    "<form method='POST' action='/setup'>Network: <input name='ssid' required>"
    "<p>Password: <input name='password' type='password' required></p>"
    "<button>Save and restart</button></form></body></html>");
}

static void cdSaveSetup() {
  if (!cdSetupMode || !ui.hasArg("ssid") || !ui.hasArg("password") ||
      ui.arg("ssid").length() < 1 || ui.arg("ssid").length() > 32 ||
      ui.arg("password").length() < 8 || ui.arg("password").length() > 63) {
    ui.send(400, "text/plain", "Invalid Wi-Fi settings"); return;
  }
  cdNetworkPrefs.putString("ssid", ui.arg("ssid"));
  cdNetworkPrefs.putString("password", ui.arg("password"));
  ui.send(200, "text/plain", "Saved. Restarting.");
  cdRestartPending = true;
  cdRestartAt = millis() + 1500;
}

static void cdFirmwareRoutes() {
  ui.on("/api/firmware", HTTP_GET, cdFirmwareInfo);
  ui.on("/update", HTTP_POST, cdFirmwareUploadFinish, cdFirmwareUploadData);
  ui.on("/setup", HTTP_GET, cdSetupPage);
  ui.on("/setup", HTTP_POST, cdSaveSetup);
}

static void cdFirmwareTick() {
  if (cdRestartPending && (int32_t)(millis() - cdRestartAt) >= 0) {
    delay(50);
    ESP.restart();
  }
}
