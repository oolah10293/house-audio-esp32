/*
 * Network CD source - milestone 1: USB CD drive identity and TOC probe.
 * Source-only prototype for Seeed XIAO ESP32-S3.
 *
 * Upstream transport driver: julpage/ESP32_CD_Player (WTFPL v2).
 * No DAC, Wi-Fi, Snapcast or playback is initialized in this milestone.
 *
 * IMPORTANT: The USB-C port is used as HOST. Use ESP32 UART0
 * (XIAO D6 / GPIO43 TX, D7 / GPIO44 RX) for serial logging.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "usbhost_driver.h"
#include "usbhost_scsi_cmd.h"

#define POLL_MS 1500
#define TOC_CAPACITY (4u + 8u * 100u)
static const char *TAG = "cd_probe";
static uint8_t toc[TOC_CAPACITY];

static uint16_t big16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t big32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void ascii_field(char *dst, size_t dst_size,
                        const uint8_t *src, size_t src_size) {
    size_t n = src_size < dst_size - 1 ? src_size : dst_size - 1;
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i] >= 32 && src[i] <= 126 ? (char)src[i] : '?';
    }
    while (n > 0 && dst[n - 1] == ' ') n--;
    dst[n] = '\0';
}

static void describe_drive(void) {
    uint8_t data[36] = {0};
    uint32_t size = sizeof(data);
    esp_err_t err = usbhost_scsi_inquiry(data, &size);
    if (err != ESP_OK || size < 32) {
        ESP_LOGW(TAG, "SCSI INQUIRY failed: %s (bytes=%lu)",
                 esp_err_to_name(err), (unsigned long)size);
        return;
    }

    char vendor[9], product[17], revision[5];
    ascii_field(vendor, sizeof vendor, data + 8, 8);
    ascii_field(product, sizeof product, data + 16, 16);
    if (size >= 36) {
        ascii_field(revision, sizeof revision, data + 32, 4);
    } else {
        strcpy(revision, "n/a");
    }
    ESP_LOGI(TAG, "SCSI INQUIRY: %s %s rev %s, peripheral type 0x%02x",
             vendor, product, revision, data[0] & 0x1f);
}

static bool list_audio_tracks(void) {
    uint8_t header[4] = {0};
    uint32_t size = sizeof header;
    esp_err_t err = usbhost_scsi_readTOC(false, 0, header, &size);
    if (err != ESP_OK || size != sizeof header) {
        ESP_LOGW(TAG, "TOC header unavailable: %s, %lu bytes",
                 esp_err_to_name(err), (unsigned long)size);
        return false;
    }

    size_t total = (size_t)big16(header) + 2u;
    if (total < 12u || total > sizeof toc || (total - 4u) % 8u != 0) {
        ESP_LOGE(TAG, "Unexpected TOC length: %u", (unsigned)total);
        return false;
    }

    size = (uint32_t)total;
    err = usbhost_scsi_readTOC(false, 0, toc, &size);
    if (err != ESP_OK || size < total) {
        ESP_LOGW(TAG, "Full TOC unavailable: %s, %lu/%u bytes",
                 esp_err_to_name(err), (unsigned long)size, (unsigned)total);
        return false;
    }

    uint8_t first = toc[2], last = toc[3];
    size_t descriptors = (total - 4u) / 8u;
    ESP_LOGI(TAG, "Disc TOC: tracks %u-%u, %u descriptors",
             first, last, (unsigned)descriptors);

    unsigned audio_count = 0;
    for (size_t i = 0; i < descriptors; ++i) {
        const uint8_t *d = toc + 4u + i * 8u;
        uint8_t track = d[2];
        if (track == 0xaa) continue; // lead-out is not a track

        bool is_audio = (d[1] & 0x04u) == 0; // MMC control bit 2: data track
        uint32_t start = big32(d + 4);

        uint32_t frames = 0;
        if (i + 1u < descriptors) {
            const uint8_t *next = toc + 4u + (i + 1u) * 8u;
            uint32_t end = big32(next + 4);
            if (end >= start) frames = end - start;
        }

        if (is_audio) ++audio_count;
        ESP_LOGI(TAG, "Track %02u: %s, LBA=%lu, duration=%02lu:%02lu.%02lu",
                 track, is_audio ? "AUDIO" : "DATA",
                 (unsigned long)start,
                 (unsigned long)(frames / (75u * 60u)),
                 (unsigned long)((frames / 75u) % 60u),
                 (unsigned long)(frames % 75u));
    }

    ESP_LOGI(TAG, "%u audio track(s) detected", audio_count);
    return audio_count > 0;
}

void app_main(void) {
    ESP_LOGI(TAG, "XIAO ESP32-S3 USB CD probe - SOURCE ONLY / FIRST MILESTONE");
    ESP_LOGI(TAG, "USB-C is host; logs require UART0 on D6/D7");
    usbhost_driverInit();

    bool drive_seen = false;
    bool disc_listed = false;
    bool media_missing_reported = false;

    while (true) {
        if (!usbhost_driverObj.deviceIsOpened) {
            if (drive_seen) {
                ESP_LOGW(TAG, "USB drive disconnected");
            }
            drive_seen = false;
            disc_listed = false;
            media_missing_reported = false;
            vTaskDelay(pdMS_TO_TICKS(POLL_MS));
            continue;
        }

        if (!drive_seen) {
            drive_seen = true;
            ESP_LOGI(TAG, "USB mass-storage device opened");
            describe_drive();
        }

        esp_err_t ready = usbhost_scsi_testUnitReady();
        if (ready != ESP_OK) {
            if (!media_missing_reported) {
                ESP_LOGI(TAG, "No ready disc yet (insert an audio CD)");
                media_missing_reported = true;
            }
            disc_listed = false;
        } else {
            media_missing_reported = false;
            if (!disc_listed) {
                disc_listed = list_audio_tracks();
                if (!disc_listed) {
                    ESP_LOGI(TAG, "Disc not yet readable as audio CD; retrying");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}
