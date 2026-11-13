# LiteOn CD player 3.3 — connection-state correction

## Authorization and baseline
- User explicitly authorized the updated firmware in the current conversation after reviewing the original `julpage/ESP32_CD_Player` project and Espressif's documented networking bug.
- Last confirmed installed firmware: **3.2**, exact commit `0fc770eb88753e5ef8a523c530f5b5741028f5aa`, running image MD5 `28fc4aea5e52d4781af59dbdd016d624`, verified by Pi one-command OTA.
- **3.2 failed hardware acceptance:** on a Track 10 -> 4 transition, the Pi-to-S3 TCP handshake and full GET for Track 4 succeeded, but the S3 sent RST without HTTP response approximately 38 ms afterward, while the old stream was closing. The Pi gateway logged HTTP 502 and MPD skipped to Track 5. See `CD_PLAYER_3_2_INSTALL.md` for both packet traces.

## Grounded library finding
- Arduino ESP32 core **2.0.14** `WiFiClient::connected()` calls `recv(fd(), &dummy, 0, MSG_DONTWAIT)` and checks `errno`, including after a zero-byte read that may not set errno.
- Espressif issue https://github.com/espressif/arduino-esp32/issues/10559 documents that zero-byte read / stale errno problem; upstream PR https://github.com/espressif/arduino-esp32/pull/10640 was merged November 25, 2024.
- `WiFiClient::operator bool()` calls that affected `connected()` routine. Our 3.2 source uses it on the newly accepted TCP socket. An erroneous disconnected result can drop the socket without HTTP response. **Causal attribution remains unverified until 3.3 physical acceptance.**
- Original `julpage/ESP32_CD_Player` reads disc audio directly to I2S, does not implement TCP streaming or track handoff. Our USB SCSI port is not the identified target for this correction.

## 3.3 source scope
- `HouseAudioCDInput/src/HouseAudioInputTransport.h`: add `audioSocketOpen()`, a one-byte, nonblocking `recv(MSG_PEEK | MSG_DONTWAIT)` check. Evaluate `errno` only if that recv actually fails; distinguish no-data-yet from EOF. This avoids use of `WiFiClient::operator bool()` or `connected()` for track socket validity checks without patching or upgrading the shared 2.0.14 toolchain.
- Keep an accepted-but-partial next-track GET alive if the old stream ends before request-line parsing finishes; carry the saved client and partial request line into the idle server path instead of abandoning it.
- Add serial diagnostics with client port and received request for acceptance, stream-replacement, partial-request handoff, and drops.
- Do not modify Pi gateway, MPD, Wi-Fi settings, CD TOC, READ CD commands, USB/SCSI timeouts, PCM/WAV format, disc-event identity, or OTA service.
- `BuildIdentity.h` and the authorized GitHub Actions build workflow use **3.3**, compile on pinned Arduino-ESP32 2.0.14 for XIAO ESP32-S3 and verify a zip manifest pinned to one commit.

## Verification status
**Uncompiled until GitHub CI passes. Hardware track-switch behavior not yet verified.** Build success and OTA identity are separate from 502 correction.

## Pi delivery
Use the existing single-command Pi OTA installer `cd-arduino/tools/install-liteon.sh` only once it has been updated to the published 3.3 release asset and its verified SHA-256 checksum. Confirm the installer still pins the original compiled commit, not a moving-branch build. No Windows ZIP transfer required.

## Field acceptance after user installs 3.3
Verify OTA-reported exact source SHA. Replay Track 10 -> 4 and Track 11 -> 6 and inspect `house-audio-input-gateway.service` journal for track-specific 502 / reset. Confirm that S3 responds `HTTP/1.1 200 OK` for the intended new track (rather than skipping) and that complete WAV playback, CD detection, autoplay, and playlist restoration still work. A source-level correction alone is not proof of success.
