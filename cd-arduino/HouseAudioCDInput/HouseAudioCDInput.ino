/*
 * House Audio CD Input: USB audio-CD source node (Arduino IDE 2.0.14).
 * Uses the tested XIAO ESP32-S3 USB MSC/SCSI driver + reusable HTTP WAV transport.
 * v0.2: resilient disc readiness / SCSI sense diagnostics and guarded drive restart.
 * Existing CD audio streaming code remains unchanged.
 * Direct home Wi-Fi only; set credentials in LocalNetworkConfig.h before uploading.
 * No ESPHome dependency; use the normal Arduino IDE Upload.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "LocalNetworkConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
extern "C" {
#include "src/usb_host_msc/usbhost_driver.h"
#include "src/usb_host_msc/usbhost_scsi_cmd.h"
}
#include "src/HouseAudioInputTransport.h"

static constexpr uint16_t GATEWAY_PORT = 8768;
static WebServer ui(80);
static SemaphoreHandle_t tocMutex;
static SemaphoreHandle_t stateMutex;
static AudioInputTrack discTracks[99];
static size_t discTrackCount = 0;
static uint8_t tocBuffer[4 + 8 * 100];
static char statusText[128] = "Starting";
static char scsiDiagnostic[180] = "No SCSI errors reported";
static uint32_t consecutiveNotReady = 0;
static uint32_t recoveryAttempts = 0;
static char bootSession[17] = {};
static uint32_t discRevision = 0;
static uint32_t discFingerprint = 0;
static bool discEverDetected = false;
static bool physicalMediaMissing = false;
static constexpr uint32_t CD_POLL_MS = 2000;
static constexpr uint32_t CD_TOC_RECHECK_MS = 15000;
static constexpr uint32_t KEEP_TRACKS_THROUGH_FAILURES = 4; // retain through brief glitches (8 seconds)
static constexpr uint32_t DRIVE_START_RETRY_MS = 30000;

static String gatewayIP, friendlyName, sourceId;
static int lastRegisterHTTP = -999; // -999 means not attempted yet
static uint32_t lastRegister = 0, lastReconnect = 0;
static volatile bool rescan = false;

static uint16_t be16(const uint8_t *p) { return (uint16_t(p[0]) << 8) | p[1]; }
static uint32_t be32(const uint8_t *p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
         (uint32_t(p[2]) << 8) | p[3];
}

static void setStatus(const char *text) {
  bool changed = false;
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    changed = (strncmp(statusText, text, sizeof statusText) != 0);
    snprintf(statusText, sizeof statusText, "%s", text);
    xSemaphoreGive(stateMutex);
  }
  if (changed) Serial.printf("[CD] %s\n", text);
}

static void setScsiDiagnostic(const char *text, uint32_t failures, uint32_t attempts) {
  bool changed = false;
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    changed = (strncmp(scsiDiagnostic, text, sizeof scsiDiagnostic) != 0);
    snprintf(scsiDiagnostic, sizeof scsiDiagnostic, "%s", text);
    consecutiveNotReady = failures;
    recoveryAttempts = attempts;
    xSemaphoreGive(stateMutex);
  }
  if (changed) Serial.printf("[CD/SCSI] %s\n", text);
}

// Request SCSI sense only after a failed readiness command, never during normal reads.
// If sense retrieval itself fails, report its transport error instead of inventing codes.
static bool explainNotReady(esp_err_t readyError, char *out, size_t size) {
  uint8_t sense[18] = {};
  esp_err_t sr = usbhost_scsi_requestSense(sense);
  if (sr != ESP_OK) {
    snprintf(out, size, "TEST UNIT READY %s (0x%X); REQUEST SENSE %s (0x%X)",
             esp_err_to_name(readyError), (unsigned)readyError,
             esp_err_to_name(sr), (unsigned)sr);
    return false;
  }
  uint8_t format = sense[0] & 0x7f;
  uint8_t key, asc, ascq;
  if (format == 0x70 || format == 0x71) {
    key = sense[2] & 0x0f; asc = sense[12]; ascq = sense[13];
  } else if (format == 0x72 || format == 0x73) {
    key = sense[1] & 0x0f; asc = sense[2]; ascq = sense[3];
  } else {
    snprintf(out, size, "TEST UNIT READY %s (0x%X); unknown sense format 0x%02X",
             esp_err_to_name(readyError), (unsigned)readyError, (unsigned)format);
    return false;
  }
  snprintf(out, size, "TEST UNIT READY %s (0x%X); sense key 0x%02X ASC 0x%02X ASCQ 0x%02X",
           esp_err_to_name(readyError), (unsigned)readyError,
           (unsigned)key, (unsigned)asc, (unsigned)ascq);
  return key == 0x02 && asc == 0x3a; // NOT READY / MEDIUM NOT PRESENT
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
  if (err != ESP_OK || size != sizeof head) {
    char note[160];
    snprintf(note, sizeof note, "READ TOC header error: %s (0x%X), %lu/4 bytes",
             esp_err_to_name(err), (unsigned)err, (unsigned long)size);
    setScsiDiagnostic(note, 0, recoveryAttempts);
    return false;
  }
  const size_t total = be16(head) + 2u;
  if (total < 12 || total > sizeof tocBuffer || ((total - 4) % 8) != 0) {
    setScsiDiagnostic("READ TOC returned an invalid table length", 0, recoveryAttempts);
    return false;
  }
  size = (uint32_t)total;
  err = usbhost_scsi_readTOC(false, 0, tocBuffer, &size);
  if (err != ESP_OK || size < total) {
    char note[160];
    snprintf(note, sizeof note, "READ TOC data error: %s (0x%X), %lu/%u bytes",
             esp_err_to_name(err), (unsigned)err, (unsigned long)size, (unsigned)total);
    setScsiDiagnostic(note, 0, recoveryAttempts);
    return false;
  }
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
  // Fingerprint the TOC, not just the track count. A new disc can have
  // exactly the same number of tracks as the previous one.
  uint32_t fingerprint = 2166136261u;
  for (size_t i = 0; i < total; ++i) {
    fingerprint ^= tocBuffer[i];
    fingerprint *= 16777619u;
  }
  if (xSemaphoreTake(tocMutex, pdMS_TO_TICKS(250)) != pdTRUE) return false;
  if (count && (!discEverDetected || physicalMediaMissing || fingerprint != discFingerprint)) {
    ++discRevision;
    if (discRevision == 0) ++discRevision;
  }
  if (count) {
    discEverDetected = true;
    discFingerprint = fingerprint;
    physicalMediaMissing = false;
  }
  memcpy(discTracks, fresh, count * sizeof(AudioInputTrack));
  discTrackCount = count;
  xSemaphoreGive(tocMutex);
  if (!count) setScsiDiagnostic("READ TOC succeeded but no audio tracks found", 0, recoveryAttempts);
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
  uint32_t failures = 0, attempts = 0, lastStartAttempt = 0;
  bool startAttempted = false;
  uint32_t lastTocScan = 0;
  for (;;) {
    if (!usbhost_driverObj.deviceIsOpened) {
      if (hadDrive) {
        physicalMediaMissing = true;
        forgetDisc();
        setStatus("USB drive disconnected");
      }
      hadDrive = false;
      haveTracks = false;
      failures = attempts = 0;
      startAttempted = false;
      setScsiDiagnostic("Waiting for USB drive", 0, 0);
    } else {
      if (!hadDrive) {
        hadDrive = true;
        failures = attempts = 0;
        startAttempted = false;
        setStatus("USB CD drive connected; checking disc");
      }
      if (rescan) {
        rescan = false;
        forgetDisc();
        haveTracks = false;
        failures = 0;
        setStatus("Rescanning CD");
      }
      esp_err_t ready = usbhost_scsi_testUnitReady();
      if (ready == ESP_OK) {
        bool wasRecovering = failures > 0;
        failures = 0;
        // Re-read the TOC after recovery to avoid exposing a cached list
        // from a disc that may have been swapped while the drive was unready.
        // Catch quick swaps between readiness polls without interrogating
        // the drive during active audio extraction.
        bool scheduled = millis() - lastTocScan >= CD_TOC_RECHECK_MS
                         && !audioInputIsStreaming();
        if (!haveTracks || wasRecovering || scheduled) {
          lastTocScan = millis();
          if (readDiscToc()) {
            haveTracks = true;
            setScsiDiagnostic(wasRecovering ? "Drive recovered; TOC refreshed" : "Drive ready; TOC read OK", 0, attempts);
            setStatus("Audio CD detected");
          } else {
            // A single TOC failure must not erase known-good tracks.
            if (!haveTracks) setStatus("Drive ready; retrying CD track read");
            else setStatus("Drive ready; retrying TOC refresh");
          }
        }
      } else {
        if (failures < 0xffffffffu) ++failures;
        char note[180];
        bool noMedium = explainNotReady(ready, note, sizeof note);
        if (noMedium) physicalMediaMissing = true;
        setScsiDiagnostic(note, failures, attempts);
        if (haveTracks && failures <= KEEP_TRACKS_THROUGH_FAILURES) {
          setStatus("CD briefly not ready; keeping tracks while retrying");
        } else {
          if (haveTracks) { forgetDisc(); haveTracks = false; }
          setStatus(noMedium ? "No CD in drive; waiting for disc" : "CD not ready; automatic recovery active");
        }
        // Try the non-ejecting SCSI START command when the drive remains
        // unready; limit to once per 30 seconds, never when sense says
        // "medium not present". Do not reset the USB bus while WAV reads
        // may be in flight. Subsequent polling rechecks readiness and TOC.
        if (!noMedium && failures >= 3 &&
            (!startAttempted || millis() - lastStartAttempt >= DRIVE_START_RETRY_MS)) {
          lastStartAttempt = millis();
          startAttempted = true;
          ++attempts;
          esp_err_t startErr = usbhost_scsi_startStopUnit(false, true);
          char recovery[180];
          snprintf(recovery, sizeof recovery,
                   "Drive start attempt %lu: %.32s (0x%X); last %.70s",
                   (unsigned long)attempts, esp_err_to_name(startErr),
                   (unsigned)startErr, note);
          setScsiDiagnostic(recovery, failures, attempts);
          setStatus(startErr == ESP_OK ? "Requested CD drive start; retrying"
                                        : "CD start command failed; retrying");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(CD_POLL_MS));
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
  char lastError[sizeof scsiDiagnostic];
  uint32_t failCount = 0, starts = 0;
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    memcpy(stage, statusText, sizeof stage);
    memcpy(lastError, scsiDiagnostic, sizeof lastError);
    failCount = consecutiveNotReady;
    starts = recoveryAttempts;
    xSemaphoreGive(stateMutex);
  } else {
    snprintf(stage, sizeof stage, "Busy");
    snprintf(lastError, sizeof lastError, "Busy");
  }
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
  page += F("<p>SCSI diagnostic: "); page += htmlEscape(String(lastError));
  page += F("</p><p>Consecutive not-ready checks: "); page += String(failCount);
  page += F("</p><p>Automatic drive-start attempts: "); page += String(starts);
  page += F("</p><p>Boot session: "); page += bootSession;
  page += F("</p><p>Disc revision: "); page += String(discRevision);
  page += F("</p><h3>CD tracks</h3><ul>");
  for (size_t i = 0; i < n; i++) {
    page += "<li><a href='http://" + WiFi.localIP().toString() + ":8080/track/" + String(tracks[i].number) + ".wav'>Track " + String(tracks[i].number) + "</a></li>";
  }
  page += F("</ul><form method='POST' action='/rescan'><button>Rescan CD</button></form>");
  page += F("<h3>Network</h3><p>Home Wi-Fi SSID: ");
  page += htmlEscape(String(HOME_WIFI_SSID));
  page += F("</p><p>Pi registration: ");
  if (lastRegisterHTTP == 200) page += F("OK (HTTP 200)");
  else if (lastRegisterHTTP == -999) page += F("not attempted yet");
  else page += "HTTP " + String(lastRegisterHTTP);
  page += F("</p><p>Wi-Fi settings are compiled locally from LocalNetworkConfig.h.</p>");
  page += F("<p>CD audio input is experimental. It does not automatically change House Music playback.</p></body></html>");
  ui.send(200, "text/html", page);
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
                "\",\"bootSession\":\"" + String(bootSession) + "\",\"discRevision\":" +
                String(discRevision) + ",\"port\":8080,\"tracks\":[";
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
    lastRegisterHTTP = result;
    if (result != 200) Serial.printf("[CD] Pi registration HTTP %d\n", result);
    http.end();
  } else {
    lastRegisterHTTP = -998; // unable to start HTTP connection
  }
}

void setup() {
  Serial.begin(115200);
  tocMutex = xSemaphoreCreateMutex();
  stateMutex = xSemaphoreCreateMutex();
  if (!tocMutex || !stateMutex) { Serial.println("Mutex allocation failed"); return; }
  gatewayIP = PI_GATEWAY_IP;
  friendlyName = SOURCE_DISPLAY_NAME;
  // New nonce on every power-on, unchanged during normal heartbeats.
  snprintf(bootSession, sizeof bootSession, "%08lx%08lx",
           (unsigned long)esp_random(), (unsigned long)esp_random());
  sourceId = "cd-" + String((uint32_t)(ESP.getEfuseMac() >> 24), HEX) +
             String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFULL), HEX);

  WiFi.mode(WIFI_STA);
  WiFi.begin(HOME_WIFI_SSID, HOME_WIFI_PASSWORD);
  ui.on("/", HTTP_GET, statusPage);
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
  if (millis() - lastReconnect > 10000) {
    lastReconnect = millis();
    if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  }
  if (millis() - lastRegister > 5000) {
    lastRegister = millis();
    registerWithPi();
  }
  delay(10);
}
