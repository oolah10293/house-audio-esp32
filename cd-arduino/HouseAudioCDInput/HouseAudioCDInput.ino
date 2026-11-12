/*
 * House Audio CD Input: USB audio-CD source node (Arduino IDE 2.0.14).
 * Uses the tested XIAO ESP32-S3 USB MSC/SCSI driver + reusable HTTP WAV transport.
 * This is a new, untested source, not a replacement for the proven NetworkCDProbe.
 * No ESPHome dependency, no prebuilt .bin flashing; use the normal Arduino Upload.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
extern "C" {
#include "src/usb_host_msc/usbhost_driver.h"
#include "src/usb_host_msc/usbhost_scsi_cmd.h"
}
#include "src/HouseAudioInputTransport.h"

static constexpr char AP_NAME[] = "House-CD-Setup";
static constexpr char AP_PASSWORD[] = "cdprobe1633";
static constexpr uint16_t GATEWAY_PORT = 8768;
static WebServer ui(80);
static Preferences prefs;
static SemaphoreHandle_t tocMutex;
static SemaphoreHandle_t stateMutex;
static AudioInputTrack discTracks[99];
static size_t discTrackCount = 0;
static uint8_t tocBuffer[4 + 8 * 100];
static char statusText[128] = "Starting";
static String wifiName, wifiPassword, gatewayIP, friendlyName, sourceId;
static bool restartPending = false;
static uint32_t restartRequestedAt = 0;
static uint32_t lastRegister = 0, lastReconnect = 0;
static volatile bool rescan = false;

static uint16_t be16(const uint8_t *p) { return (uint16_t(p[0]) << 8) | p[1]; }
static uint32_t be32(const uint8_t *p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
         (uint32_t(p[2]) << 8) | p[3];
}

static void setStatus(const char *text) {
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    snprintf(statusText, sizeof statusText, "%s", text);
    xSemaphoreGive(stateMutex);
  }
  Serial.printf("[CD] %s\n", text);
}

static size_t snapshotTracks(AudioInputTrack *out, size_t capacity) {
  size_t n = 0;
  if (xSemaphoreTake(tocMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
    n = discTrackCount < capacity ? discTrackCount : capacity;
    memcpy(out, discTracks, n * sizeof(AudioInputTrack));
    xSemaphoreGive(tocMutex);
  }
  return n;
}

static void forgetDisc() {
  if (xSemaphoreTake(tocMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    discTrackCount = 0;
    xSemaphoreGive(tocMutex);
  }
}

static bool readDiscToc() {
  uint8_t head[4] = {};
  uint32_t size = sizeof head;
  esp_err_t err = usbhost_scsi_readTOC(false, 0, head, &size);
  if (err != ESP_OK || size != sizeof head) return false;
  const size_t total = be16(head) + 2u;
  if (total < 12 || total > sizeof tocBuffer || ((total - 4) % 8) != 0) return false;
  size = (uint32_t)total;
  err = usbhost_scsi_readTOC(false, 0, tocBuffer, &size);
  if (err != ESP_OK || size < total) return false;
  const size_t entries = (total - 4) / 8;
  AudioInputTrack fresh[99];
  size_t count = 0;
  for (size_t i = 0; i + 1 < entries; ++i) {
    const uint8_t *desc = tocBuffer + 4 + i * 8;
    const uint8_t *next = desc + 8;
    if (desc[2] == 0xAA || (desc[1] & 0x04) != 0) continue;
    uint32_t start = be32(desc + 4), end = be32(next + 4);
    if (end <= start || desc[2] < 1 || desc[2] > 99) continue;
    if (count < 99) fresh[count++] = {desc[2], start, end};
  }
  if (xSemaphoreTake(tocMutex, pdMS_TO_TICKS(250)) != pdTRUE) return false;
  memcpy(discTracks, fresh, count * sizeof(AudioInputTrack));
  discTrackCount = count;
  xSemaphoreGive(tocMutex);
  return count > 0;
}

// Hardware-specific callback. The common transport only deals in PCM frames.
static bool readCDFrames(uint32_t lba, uint32_t frames, uint8_t *buffer, size_t capacity) {
  if (!usbhost_driverObj.deviceIsOpened || frames == 0 || capacity < frames * 2352U) return false;
  uint32_t got = 0;
  esp_err_t err = usbhost_scsi_readCD(lba, buffer, &frames, &got);
  return err == ESP_OK && got == frames * 2352U;
}

static void discTask(void *) {
  bool hadDrive = false, haveTracks = false;
  for (;;) {
    if (!usbhost_driverObj.deviceIsOpened) {
      if (hadDrive) { forgetDisc(); setStatus("USB drive disconnected"); }
      hadDrive = false;
      haveTracks = false;
    } else {
      if (!hadDrive) { hadDrive = true; setStatus("USB CD drive connected"); }
      if (rescan) { rescan = false; forgetDisc(); haveTracks = false; }
      if (usbhost_scsi_testUnitReady() != ESP_OK) {
        if (haveTracks) { forgetDisc(); setStatus("CD removed or drive not ready"); }
        haveTracks = false;
      } else if (!haveTracks) {
        if (readDiscToc()) { haveTracks = true; setStatus("Audio CD detected"); }
        else setStatus("Waiting for readable audio CD");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

static String htmlEscape(String s) {
  s.replace("&", "&amp;"); s.replace("<", "&lt;");
  s.replace(">", "&gt;"); s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

static void statusPage() {
  char stage[sizeof statusText];
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    memcpy(stage, statusText, sizeof stage);
    xSemaphoreGive(stateMutex);
  } else snprintf(stage, sizeof stage, "Busy");
  AudioInputTrack tracks[99];
  size_t n = snapshotTracks(tracks, 99);
  String page;
  page.reserve(3500);
  page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<meta http-equiv='refresh' content='8'><style>body{max-width:740px;margin:20px auto;"
            "padding:0 16px;font:16px system-ui;background:#111;color:#eee}input{padding:8px}"
            "button{padding:10px}a{color:#8df}</style></head><body><h2>House Music CD Input</h2>");
  page += F("<p>Status: "); page += htmlEscape(stage); page += F("</p><p>Source ID: ");
  page += htmlEscape(sourceId); page += F("</p><p>Wi-Fi: ");
  page += WiFi.status() == WL_CONNECTED ? htmlEscape(WiFi.localIP().toString()) : String("not connected");
  page += F("</p><p>Pi gateway: "); page += htmlEscape(gatewayIP); page += F(":8768</p>");
  page += F("<h3>CD tracks</h3><ul>");
  for (size_t i = 0; i < n; i++) {
    page += "<li><a href='http://" + WiFi.localIP().toString() + ":8080/track/" + String(tracks[i].number) + ".wav'>Track " + String(tracks[i].number) + "</a></li>";
  }
  page += F("</ul><form method='POST' action='/rescan'><button>Rescan CD</button></form>");
  page += F("<h3>Network settings</h3><form method='POST' action='/configure'>"
            "<p>Home Wi-Fi SSID <input name='ssid' value='");
  page += htmlEscape(wifiName); page += F("'></p><p>Wi-Fi password <input type='password' name='password' placeholder='Keep existing if blank'></p>"
       "<p>Raspberry Pi IP <input name='gateway' value='");
  page += htmlEscape(gatewayIP); page += F("'></p><p>Friendly name <input name='name' value='");
  page += htmlEscape(friendlyName); page += F("'></p><button>Save & restart</button></form>");
  page += F("<p>CD audio input is experimental. It does not automatically change House Music playback.</p></body></html>");
  ui.send(200, "text/html", page);
}

static void saveSettings() {
  String ssid = ui.arg("ssid"), pass = ui.arg("password");
  String gateway = ui.arg("gateway"), name = ui.arg("name");
  ssid.trim(); gateway.trim(); name.trim();
  IPAddress ip;
  if (ssid.length() < 1 || ssid.length() > 32 || !ip.fromString(gateway) ||
      name.length() < 1 || name.length() > 60) {
    ui.send(400, "text/plain", "Provide a Wi-Fi SSID, a valid Pi IPv4 address and a friendly name");
    return;
  }
  prefs.putString("ssid", ssid);
  if (pass.length()) prefs.putString("password", pass);
  prefs.putString("gateway", gateway);
  prefs.putString("name", name);
  ui.send(200, "text/plain", "Saved. Restarting; connect to home Wi-Fi to see input on Pi.");
  restartPending = true;
  restartRequestedAt = millis();
}

static void registerWithPi() {
  if (WiFi.status() != WL_CONNECTED || gatewayIP.isEmpty()) return;
  AudioInputTrack tracks[99];
  size_t n = snapshotTracks(tracks, 99);
  String safeName = friendlyName;
  safeName.replace("\\", "\\\\"); safeName.replace("\"", "\\\"");
  safeName.replace("\n", " "); safeName.replace("\r", " ");
  String json = "{\"sourceId\":\"" + sourceId + "\",\"name\":\"" + safeName +
                "\",\"kind\":\"cd\",\"status\":\"" + (n ? String("ready") : String("no_disc")) +
                "\",\"port\":8080,\"tracks\":[";
  for (size_t i = 0; i < n; i++) {
    if (i) json += ',';
    json += String(tracks[i].number);
  }
  json += "]}";
  WiFiClient net;
  HTTPClient http;
  String url = "http://" + gatewayIP + ":" + String(GATEWAY_PORT) + "/register";
  http.setTimeout(1500);
  if (http.begin(net, url)) {
    http.addHeader("Content-Type", "application/json");
    int result = http.POST(json);
    if (result != 200) Serial.printf("[CD] Pi registration HTTP %d\n", result);
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  tocMutex = xSemaphoreCreateMutex();
  stateMutex = xSemaphoreCreateMutex();
  if (!tocMutex || !stateMutex) { Serial.println("Mutex allocation failed"); return; }
  prefs.begin("house-source", false);
  wifiName = prefs.getString("ssid", "");
  wifiPassword = prefs.getString("password", "");
  gatewayIP = prefs.getString("gateway", "");
  friendlyName = prefs.getString("name", "CD Player");
  sourceId = "cd-" + String((uint32_t)(ESP.getEfuseMac() >> 24), HEX) +
             String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFULL), HEX);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  if (!wifiName.isEmpty()) WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
  ui.on("/", HTTP_GET, statusPage);
  ui.on("/configure", HTTP_POST, saveSettings);
  ui.on("/rescan", HTTP_POST, []() {
    rescan = true;
    ui.sendHeader("Location", "/"); ui.send(303, "text/plain", "Rescanning");
  });
  ui.begin();
  usbhost_driverInit();
  audioInputStartServer(snapshotTracks, readCDFrames, {44100, 2, 16, 2352}); // common audio source transport
  if (xTaskCreatePinnedToCore(discTask, "cd-disc", 8192, nullptr, 1, nullptr, 0) != pdPASS)
    setStatus("Cannot start CD task");
  else setStatus("Waiting for USB CD drive");
}

void loop() {
  ui.handleClient();
  if (restartPending && millis() - restartRequestedAt > 800) ESP.restart();
  if (millis() - lastReconnect > 10000) {
    lastReconnect = millis();
    if (WiFi.status() != WL_CONNECTED && !wifiName.isEmpty()) WiFi.reconnect();
  }
  if (millis() - lastRegister > 5000) {
    lastRegister = millis();
    registerWithPi();
  }
  delay(10);
}
