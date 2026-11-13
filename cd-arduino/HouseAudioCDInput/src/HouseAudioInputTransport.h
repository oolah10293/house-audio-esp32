#pragma once
/* Reusable, hardware-independent audio input transport for Arduino-ESP32.
 * All sampling and block parameters are provided by the hardware adapter.
 * Current milestone supports finite PCM tracks, not indefinite line-in capture.
 * Each source provides tracks, PCM format, and read-block callbacks; no hardware IDs.
 */
#include <Arduino.h>
#include <WiFi.h>

struct AudioInputTrack {
  uint8_t number;
  uint32_t startBlock; // inclusive device-defined PCM block index
  uint32_t endBlock;   // exclusive
};
using AudioTrackSnapshot = size_t (*)(AudioInputTrack*, size_t);
using AudioBlockReader = bool (*)(uint32_t, uint32_t, uint8_t*, size_t);
// Optional source-specific lease covers the entire stream, not just each read block.
using AudioStreamBegin = bool (*)();
using AudioStreamEnd = void (*)();
struct AudioPcmFormat {
  uint32_t sampleRate;
  uint16_t channels;
  uint16_t bitsPerSample;
  uint32_t bytesPerBlock;
};

static AudioTrackSnapshot audioSnapshot;
static AudioBlockReader audioReader;
static AudioStreamBegin audioBegin;
static AudioStreamEnd audioEnd;
static AudioPcmFormat audioFormat;
static WiFiServer audioServer(8080);

static bool audioWriteAll(WiFiClient &client, const uint8_t *data, size_t count) {
  unsigned long lastProgress = millis();
  while (count) {
    if (!client.connected()) return false;
    size_t wrote = client.write(data, count);
    if (wrote) {
      data += wrote;
      count -= wrote;
      lastProgress = millis();
    } else {
      if (millis() - lastProgress > 4000) return false;
      delay(2);
    }
  }
  return true;
}

static void audioLE16(uint8_t *p, uint16_t v) { p[0] = v & 255; p[1] = v >> 8; }
static void audioLE32(uint8_t *p, uint32_t v) {
  p[0] = v & 255; p[1] = (v >> 8) & 255;
  p[2] = (v >> 16) & 255; p[3] = v >> 24;
}

static void audioWavHeader(uint8_t *h, uint32_t pcmBytes) {
  const uint16_t blockAlign = audioFormat.channels * (audioFormat.bitsPerSample / 8);
  memcpy(h, "RIFF", 4); audioLE32(h + 4, 36 + pcmBytes);
  memcpy(h + 8, "WAVEfmt ", 8); audioLE32(h + 16, 16);
  audioLE16(h + 20, 1); audioLE16(h + 22, audioFormat.channels);
  audioLE32(h + 24, audioFormat.sampleRate); audioLE32(h + 28, audioFormat.sampleRate * blockAlign);
  audioLE16(h + 32, blockAlign); audioLE16(h + 34, audioFormat.bitsPerSample);
  memcpy(h + 36, "data", 4); audioLE32(h + 40, pcmBytes);
}

// The server has one streaming task. A slow or still-connected old WAV
// response must not hold a newly requested CD track behind it for minutes.
// A HEAD probe while playing is rejected without interrupting the music.
static bool audioCheckReplacement(WiFiClient &pending) {
  WiFiClient incoming = audioServer.available();
  if (!incoming) return false;
  if (incoming.peek() == 'G') {
    pending = incoming;  // process this GET after the previous USB lease ends
    return true;
  }
  incoming.print("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  incoming.stop();
  return false;
}

static void audioStreamTask(void *) {
  static uint8_t pcm[8 * 2352]; // static buffer; validate block size before use
  audioServer.begin();
  WiFiClient pending;
  for (;;) {
    WiFiClient client = pending ? pending : audioServer.available();
    pending = WiFiClient();
    if (!client) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
    client.setTimeout(1500);
    String request = client.readStringUntil('\n');
    bool head = request.startsWith("HEAD ");
    int begin = request.indexOf("/track/");
    int end = request.indexOf(".wav", begin);
    int number = -1;
    if ((request.startsWith("GET ") || head) && begin == 4 + (head ? 1 : 0) && end > begin + 7)
      number = request.substring(begin + 7, end).toInt();
    // Drain HTTP request headers so no pipelined bytes are mistaken as audio.
    for (int j = 0; j < 40 && client.connected(); ++j) {
      String line = client.readStringUntil('\n');
      if (line == "\r" || line.isEmpty()) break;
    }
    AudioInputTrack list[99], track = {};
    size_t n = audioSnapshot(list, 99);
    bool found = false;
    for (size_t i = 0; i < n; i++) if (list[i].number == number) { track = list[i]; found = true; break; }
    if (!found || track.endBlock <= track.startBlock || audioFormat.bytesPerBlock == 0 ||
        audioFormat.bytesPerBlock > sizeof pcm / 8 ||
        (uint64_t)(track.endBlock - track.startBlock) * audioFormat.bytesPerBlock > 0xFFFFFF00ULL) {
      client.print("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
      client.stop();
      continue;
    }
    // Acquire USB/SCSI ownership before acknowledging a GET. A background
    // disc poll must finish before the source begins a contiguous CD stream.
    const bool leased = head || !audioBegin || audioBegin();
    if (!leased) {
      client.print("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
      client.stop();
      continue;
    }
    uint32_t total = (track.endBlock - track.startBlock) * audioFormat.bytesPerBlock;
    client.printf("HTTP/1.1 200 OK\r\nContent-Type: audio/wav\r\nContent-Length: %lu\r\nAccept-Ranges: none\r\nConnection: close\r\n\r\n",
                  (unsigned long)(total + 44));
    if (!head) {
      uint8_t wav[44]; audioWavHeader(wav, total);
      if (audioWriteAll(client, wav, sizeof wav)) {
        for (uint32_t block = track.startBlock; block < track.endBlock; ) {
          // MPD may request another track before the old socket notices its
          // disconnect. Cancel at block boundaries so the next GET does not
          // time out behind the previous full-length CD stream.
          if (audioCheckReplacement(pending)) {
            Serial.printf("[Audio Input] switching streams at block %lu\n", (unsigned long)block);
            break;
          }
          uint32_t blocks = (track.endBlock - block < 8) ? track.endBlock - block : 8;
          if (!audioReader(block, blocks, pcm, sizeof pcm) ||
              !audioWriteAll(client, pcm, blocks * audioFormat.bytesPerBlock)) {
            Serial.printf("[Audio Input] source/read stream failed at block %lu\n", (unsigned long)block);
            break;
          }
          block += blocks;
        }
      }
    }
    client.stop();
    if (!head && audioEnd) audioEnd();
  }
}

static void audioInputStartServer(AudioTrackSnapshot snapshot, AudioBlockReader reader, AudioPcmFormat format,
                                  AudioStreamBegin begin = nullptr, AudioStreamEnd end = nullptr) {
  audioSnapshot = snapshot;
  audioReader = reader;
  audioBegin = begin;
  audioEnd = end;
  audioFormat = format;
  xTaskCreatePinnedToCore(audioStreamTask, "audio-input", 8192, nullptr, 2, nullptr, 1);
}
