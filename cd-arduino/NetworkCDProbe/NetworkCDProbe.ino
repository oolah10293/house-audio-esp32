/*
  NetworkCDProbe — Arduino IDE sketch for XIAO ESP32-S3.

  USB Host -> Lite-On SOHW-1633SX -> read audio CD track list.
  Test status in browser via the board's temporary Wi-Fi access point.

  This milestone does NOT stream audio or modify House Music.
  Source only: NOT COMPILED IN ARDUINO, NOT FLASHED, NOT HARDWARE TESTED.

  Uses the ESP-IDF USB Host library bundled with Arduino-ESP32,
  NOT the Arduino TinyUSB USBHost stack. Do not use USBHost.begin().

  Board: Seeed XIAO ESP32S3
  ESP32 board package: use Arduino-ESP32 3.3.12 (current verified stable).
  Tools > USB CDC On Boot: DISABLED.
  Tools > USB Mode: Hardware CDC and JTAG (NOT TinyUSB OTG device mode).
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

extern "C" {
#include "src/usb_host_msc/usbhost_driver.h"
#include "src/usb_host_msc/usbhost_scsi_cmd.h"
}

static constexpr char AP_NAME[] = "CD-Probe-S3";
static constexpr char AP_PASSWORD[] = "cdprobe1633";
static constexpr uint32_t POLL_INTERVAL_MS = 1500;

static WebServer web(80);
static SemaphoreHandle_t statusLock = nullptr;
static volatile bool forceRescan = false;

struct ProbeStatus {
  char stage[100];
  char description[180];
  char tracks[7000];
};
static ProbeStatus currentStatus;
static ProbeStatus webCopy;

static uint8_t tocBuffer[4 + 8 * 100];

static uint16_t fromBig16(const uint8_t *p) {
  return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t fromBig32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void copyText(char *dst, size_t capacity, const char *text) {
  if (!text) text = "";
  snprintf(dst, capacity, "%s", text);
}

static void publish(const char *stage, const char *description, const char *tracks = "") {
  if (xSemaphoreTake(statusLock, pdMS_TO_TICKS(500)) != pdTRUE) return;
  copyText(currentStatus.stage, sizeof currentStatus.stage, stage);
  copyText(currentStatus.description, sizeof currentStatus.description, description);
  copyText(currentStatus.tracks, sizeof currentStatus.tracks, tracks);
  xSemaphoreGive(statusLock);

  Serial.printf("[CD] %s: %s\n", stage, description ? description : "");
}

static void safeAscii(char *dst, size_t capacity, const uint8_t *src, size_t length) {
  if (capacity == 0) return;
  size_t n = length < capacity - 1 ? length : capacity - 1;
  for (size_t i = 0; i < n; i++) {
    uint8_t v = src[i];
    dst[i] = v >= 32 && v <= 126 ? (char)v : '?';
  }
  while (n > 0 && dst[n - 1] == ' ') --n;
  dst[n] = '\0';
}

static void readDriveIdentity(char *dest, size_t length) {
  uint8_t inquiry[36] = {};
  uint32_t bytes = sizeof inquiry;
  esp_err_t result = usbhost_scsi_inquiry(inquiry, &bytes);

  if (result != ESP_OK || bytes < 32) {
    snprintf(dest, length, "USB device opened; SCSI INQUIRY failed (%s, %lu bytes)",
             esp_err_to_name(result), (unsigned long)bytes);
    return;
  }

  char vendor[9], product[17], revision[5];
  safeAscii(vendor, sizeof vendor, inquiry + 8, 8);
  safeAscii(product, sizeof product, inquiry + 16, 16);
  if (bytes >= 36) safeAscii(revision, sizeof revision, inquiry + 32, 4);
  else copyText(revision, sizeof revision, "?");
  snprintf(dest, length, "%s %s (revision %s, type 0x%02X)",
           vendor, product, revision, (unsigned)(inquiry[0] & 0x1F));
}

static bool readAudioDisc(char *list, size_t capacity, char *description, size_t descCapacity) {
  list[0] = '\0';

  uint8_t header[4] = {};
  uint32_t size = sizeof header;
  esp_err_t err = usbhost_scsi_readTOC(false, 0, header, &size);
  if (err != ESP_OK || size != sizeof header) {
    snprintf(description, descCapacity, "READ TOC header failed (%s, %lu bytes)",
             esp_err_to_name(err), (unsigned long)size);
    return false;
  }

  const size_t total = (size_t)fromBig16(header) + 2u;
  if (total < 12u || total > sizeof tocBuffer || ((total - 4u) % 8u) != 0) {
    snprintf(description, descCapacity, "Unexpected CD table of contents length: %u",
             (unsigned)total);
    return false;
  }

  size = (uint32_t)total;
  err = usbhost_scsi_readTOC(false, 0, tocBuffer, &size);
  if (err != ESP_OK || size < total) {
    snprintf(description, descCapacity, "READ TOC data failed (%s, %lu of %u bytes)",
             esp_err_to_name(err), (unsigned long)size, (unsigned)total);
    return false;
  }

  const size_t entries = (total - 4u) / 8u;
  unsigned audioTracks = 0, dataTracks = 0;
  size_t used = 0;

  for (size_t i = 0; i < entries; ++i) {
    const uint8_t *d = tocBuffer + 4u + 8u * i;
    const uint8_t trackNumber = d[2];
    if (trackNumber == 0xAA) continue; // lead-out descriptor

    bool audio = (d[1] & 0x04) == 0; // MMC control bit 2 marks data tracks
    uint32_t startLBA = fromBig32(d + 4);
    uint32_t frames = 0;
    if (i + 1 < entries) {
      uint32_t endLBA = fromBig32(tocBuffer + 4u + 8u * (i + 1) + 4u);
      if (endLBA >= startLBA) frames = endLBA - startLBA;
    }
    if (audio) ++audioTracks; else ++dataTracks;

    int written = snprintf(list + used, capacity - used,
                           "Track %02u  %-5s  %02lu:%02lu.%02lu  LBA %lu\n",
                           (unsigned)trackNumber, audio ? "AUDIO" : "DATA",
                           (unsigned long)(frames / (75UL * 60UL)),
                           (unsigned long)((frames / 75UL) % 60UL),
                           (unsigned long)(frames % 75UL),
                           (unsigned long)startLBA);
    if (written < 0 || (size_t)written >= capacity - used) break;
    used += (size_t)written;
  }

  snprintf(description, descCapacity, "Disc tracks %u-%u: %u audio, %u data",
           (unsigned)tocBuffer[2], (unsigned)tocBuffer[3], audioTracks, dataTracks);
  return audioTracks > 0;
}

static void probeTask(void *unused) {
  (void)unused;
  bool opened = false;
  bool tracksKnown = false;
  // Keep large buffers in static RAM rather than the probe task's stack.
  static char drive[120] = "";
  static char desc[180] = "";
  static char tracks[7000] = "";

  for (;;) {
    if (!usbhost_driverObj.deviceIsOpened) {
      if (opened) publish("Drive disconnected", "Waiting for the Lite-On USB drive");
      opened = false;
      tracksKnown = false;
      drive[0] = '\0';
      vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
      continue;
    }

    if (!opened) {
      readDriveIdentity(drive, sizeof drive);
      opened = true;
      tracksKnown = false;
      publish("Drive detected", drive);
    }

    if (forceRescan) {
      forceRescan = false;
      tracksKnown = false;
    }

    esp_err_t ready = usbhost_scsi_testUnitReady();
    if (ready != ESP_OK) {
      if (tracksKnown) tracksKnown = false;
      publish("Waiting for an audio CD", drive);
    } else if (!tracksKnown) {
      bool found = readAudioDisc(tracks, sizeof tracks, desc, sizeof desc);
      if (found) {
        tracksKnown = true;
        publish("Audio CD detected", desc, tracks);
      } else {
        publish("No readable audio tracks yet", desc);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
  }
}

static String escapeHtml(const char *s) {
  String value(s ? s : "");
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  return value;
}

static void handlePage() {
  if (xSemaphoreTake(statusLock, pdMS_TO_TICKS(500)) == pdTRUE) {
    memcpy(&webCopy, &currentStatus, sizeof currentStatus);
    xSemaphoreGive(statusLock);
  }
  String page;
  page.reserve(9000);
  page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>");
  page += F("<meta http-equiv='refresh' content='5'>");
  page += F("<title>Network CD Probe</title><style>");
  page += F("body{font:16px system-ui;max-width:760px;margin:24px auto;padding:0 16px;background:#111;color:#eee}");
  page += F("article{border:1px solid #444;border-radius:10px;padding:16px;margin:16px 0}");
  page += F("pre{white-space:pre-wrap;font:14px ui-monospace,monospace}button{padding:12px 18px;cursor:pointer}");
  page += F("</style></head><body><h2>Lite-On CD drive probe</h2><article><b>Status:</b> ");
  page += escapeHtml(webCopy.stage);
  page += F("<p>");
  page += escapeHtml(webCopy.description);
  page += F("</p></article><article><b>Tracks</b><pre>");
  page += escapeHtml(webCopy.tracks);
  page += F("</pre></article><form action='/rescan' method='post'><button>Rescan disc</button></form>");
  page += F("<p>Test only. No CD audio streaming yet. Refreshes every 5 seconds.</p></body></html>");
  web.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  statusLock = xSemaphoreCreateMutex();
  if (statusLock == nullptr) {
    Serial.println("[CD] Fatal: status mutex allocation failed");
    return;
  }

  publish("Starting", "Initializing Arduino Wi-Fi access point");
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_NAME, AP_PASSWORD)) {
    publish("Wi-Fi AP error", "Could not start the diagnostic access point");
  } else {
    Serial.printf("[CD] Wi-Fi: %s, browse http://%s\n", AP_NAME,
                   WiFi.softAPIP().toString().c_str());
  }

  web.on("/", HTTP_GET, handlePage);
  web.on("/rescan", HTTP_POST, []() {
    forceRescan = true;
    web.sendHeader("Location", "/", true);
    web.send(303, "text/plain", "Rescan requested");
  });
  web.begin();

  publish("Waiting for USB drive", "Connect the powered Lite-On to XIAO USB-C");
  // ESP-IDF host library is provided by the ESP32 Arduino board package.
  usbhost_driverInit();
  if (xTaskCreatePinnedToCore(probeTask, "cd_probe", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
    publish("Startup error", "Could not create USB CD probe task");
  }
}

void loop() {
  web.handleClient();
  delay(10);
}
