#pragma once
/* Reusable, hardware-independent audio input transport for Arduino-ESP32.
 * All sampling and block parameters are provided by the hardware adapter.
 * Current milestone supports finite PCM tracks, not indefinite line-in capture.
 * Each source provides tracks, PCM format, and read-block callbacks; no hardware IDs.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <lwip/sockets.h>
#include <errno.h>

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

// Arduino-ESP32 2.0.14 WiFiClient::connected() calls recv(fd, .., 0)
// and interprets a stale errno as a current connection error (Espressif
// issue #10559, later fixed by PR #10640). Inspect the actual socket with
// MSG_PEEK instead: no data yet means open, zero means peer FIN, and only
// the errno from a failing one-byte recv is meaningful.
static bool audioSocketOpen(WiFiClient &client) {
  int socketFd = client.fd();
  if (socketFd < 0) return false;
  uint8_t peek = 0;
  int received = recv(socketFd, &peek, 1, MSG_PEEK | MSG_DONTWAIT);
  if (received > 0) return true;
  if (received == 0) return false;
  return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR;
}

static bool audioWriteAll(WiFiClient &client, const uint8_t *data, size_t count) {
  unsigned long lastProgress = millis();
  while (count) {
    if (!audioSocketOpen(client)) return false;
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

// The single streaming task must preserve an arriving GET until the old WAV
// and its USB lease are released. A TCP accept does not guarantee the complete
// HTTP request line has arrived. The former blocking read used a 300-second
// WiFiClient timeout (mistaken for milliseconds), which could stall the handoff.
// HEAD and non-track requests are refused without interrupting playback.
static bool audioCheckReplacement(WiFiClient &candidate, String &candidateLine,
                                  unsigned long &candidateSince,
                                  WiFiClient &pending, String &pendingRequest) {
  if (!audioSocketOpen(candidate)) {
    candidate.stop();
    candidate = audioServer.available();
    if (!audioSocketOpen(candidate)) {
      candidate.stop();
      return false;
    }
    candidateLine = "";
    candidateSince = millis();
    Serial.printf("[Audio Input] accepted replacement socket port=%u\n",
                  (unsigned)candidate.remotePort());
  }

  // Do not block CD reads while waiting for the HTTP request line. Keep the
  // accepted socket and any partial bytes through subsequent audio blocks.
  while (candidate.available() > 0) {
    int next = candidate.read();
    if (next < 0) break;
    if (next == '\n') {
      if (candidateLine.startsWith("GET /track/")) {
        pendingRequest = candidateLine;
        Serial.printf("[Audio Input] replacement request port=%u %s\n",
                      (unsigned)candidate.remotePort(), candidateLine.c_str());
        pending = candidate;
        candidate = WiFiClient(); // pending now owns the waiting connection
        candidateLine = "";
        return true;
      }
      candidate.print("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
      candidate.stop();
      candidate = WiFiClient();
      candidateLine = "";
      return false;
    }
    if (candidateLine.length() >= 127) {
      candidate.print("HTTP/1.1 414 URI Too Long\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
      candidate.stop();
      candidate = WiFiClient();
      candidateLine = "";
      return false;
    }
    candidateLine += (char)next;
  }

  // A broken/incomplete connection must not permanently block another GET.
  // Four seconds is shorter than the Pi gateway's 8-second upstream timeout.
  if (!audioSocketOpen(candidate) || millis() - candidateSince > 4000UL) {
    Serial.printf("[Audio Input] drop incomplete request port=%u age=%lu ms\n",
                  (unsigned)candidate.remotePort(), (unsigned long)(millis() - candidateSince));
    candidate.stop();
    candidate = WiFiClient();
    candidateLine = "";
  }
  return false;
}

static void audioStreamTask(void *) {
  static uint8_t pcm[8 * 2352]; // static buffer; validate block size before use
  audioServer.begin();
  WiFiClient pending;
  String pendingRequest;
  WiFiClient candidate;
  String candidateLine;
  unsigned long candidateSince = 0;
  for (;;) {
    WiFiClient client;
    String request;
    bool completeRequest = false;
    if (audioSocketOpen(pending)) {
      client = pending;
      request = pendingRequest; // previous stream read a complete GET line
      completeRequest = true;
    } else if (audioSocketOpen(candidate)) {
      // The old stream can end before a replacement's request line is fully
      // received. Carry both its socket and partial line into the idle loop.
      client = candidate;
      request = candidateLine;
      Serial.printf("[Audio Input] resume partial request port=%u bytes=%u\n",
                    (unsigned)client.remotePort(), (unsigned)request.length());
    } else {
      client = audioServer.available();
    }
    pending.stop();
    pendingRequest = "";
    candidate.stop();
    candidateLine = "";
    if (!audioSocketOpen(client)) { client.stop(); vTaskDelay(pdMS_TO_TICKS(20)); continue; }
    // Arduino-ESP32 2.0.14 WiFiClient::setTimeout takes SECONDS, not ms.
    client.setTimeout(2);
    if (!completeRequest) request += client.readStringUntil('\n');
    Serial.printf("[Audio Input] serving socket port=%u %s\n",
                  (unsigned)client.remotePort(), request.c_str());
    bool head = request.startsWith("HEAD ");
    int begin = request.indexOf("/track/");
    int end = request.indexOf(".wav", begin);
    int number = -1;
    if ((request.startsWith("GET ") || head) && begin == 4 + (head ? 1 : 0) && end > begin + 7)
      number = request.substring(begin + 7, end).toInt();
    // Drain HTTP request headers so no pipelined bytes are mistaken as audio.
    for (int j = 0; j < 40 && audioSocketOpen(client); ++j) {
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
          if (audioCheckReplacement(candidate, candidateLine, candidateSince,
                                    pending, pendingRequest)) {
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
