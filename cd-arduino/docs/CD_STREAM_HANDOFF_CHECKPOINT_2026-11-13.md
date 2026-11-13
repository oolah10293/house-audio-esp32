# LiteOn CD stream handoff timing correction — 2026-11-13

## Verified hardware and Pi evidence
- User confirmed LiteOn CD player **3.1** on the ESP32-S3 after the first successful OTA: source SHA `01452e660f3e8fec5d30ba84823cc0cfb5d2d58e`, running sketch MD5 `741945608bf41301f27a87be68ee83fb`.
- House Music browser showed timeouts / HTTP 502 while new tracks often played or MPD advanced to later tracks; one user-confirmed browser Track 1->2 transition worked, a later Track 9 and subsequent transitions reported timeouts/502.
- Pi service was identified from live process and systemd output: `house-audio-input-gateway.service`, running `/usr/bin/python3 /home/ads-suck/gateway.py --host 0.0.0.0 --port 8768`. This gateway source is on the user’s Pi; the complete deployed gateway.py was not fetched from the device.
- User-pasted `gateway.py` lines 145–212 show it proxies GET to `source.ip:source.port`, requests the CD WAV and returns 502 if upstream connection fails before WAV response headers.
- User-pasted gateway journal at 13:22:02: old Track 4 reported `Broken pipe`, GET Track 10 reported `Connection reset by peer` and 502, followed by Track 11 HTTP 200. At 13:22:07 old Track 11 reported `Broken pipe`, GET Track 6 reported `Connection reset by peer` and 502, followed by Track 7 HTTP 200. Thus *the new track’s upstream connection is failing before response headers*; the old broken pipe is distinct. **Exact S3-side cause remains unverified.**

## Source investigation
- Repository: `oolah10293/house-audio-esp32` branch `fix/liteon-usb-transport-recovery`.
- Relevant firmware file: `cd-arduino/HouseAudioCDInput/src/HouseAudioInputTransport.h`.
- Original `audioCheckReplacement` accepted a TCP socket and used `incoming.setTimeout(300); incoming.readStringUntil('\\n');` while the previous track was streaming.
- **Verified upstream implementation:** Arduino-ESP32 **2.0.14** `WiFiClient::setTimeout(uint32_t seconds)` multiplies by 1000 and calls `Client::setTimeout(seconds * 1000)`. It also sets socket send and receive timeouts in seconds. Therefore the old `setTimeout(300)` allowed a **300-second** request-line wait, NOT the 300ms intended in comments. Likewise `client.setTimeout(1500)` allowed **1500 seconds** for later HTTP line/header reads, NOT 1500ms.
  - https://github.com/espressif/arduino-esp32/blob/2.0.14/libraries/WiFi/src/WiFiClient.cpp
  - https://github.com/espressif/arduino-esp32/blob/2.0.14/libraries/WiFi/src/WiFiClient.h
- Separately, the sketch's `HTTPClient http.setTimeout(1500)` uses **milliseconds**, as defined by its own class; it was deliberately left unchanged.
- The old blocking header read and very long timeouts are plausible contributors to gateway failures and track skipping. **Neither has been proven to be the sole root cause of the logged TCP resets.**

## Corrective source-only commits
- `f2bcd8d9a520a11f15956222888cbaa04a20d657`: replace blocking handoff read with **nonblocking, bounded** HTTP request-line accumulation, preserving partial TCP requests across audio blocks until the newline arrives; retain the client socket and request line through old-stream release; keep HEAD/non-track rejection without interrupting an active WAV; cap incomplete line at 127 chars and wait at most 4 seconds on an incomplete request.
- `132638bb77e1457c076bde759e8acaa6f578779b`: correct comment documenting timeout units and set the normal accepted `WiFiClient` timeout to **2 seconds** in the firmware transport.

The modifications are restricted to `HouseAudioInputTransport.h`. CD/SCSI sector reader, per-stream USB lease, WAV generator, boot/firmware identity, Pi gateway source, and OTA updater were not changed.

## Source verification (no compiler)
Reviewed exact GitHub diffs and committed source, including these ten source-level invariants: fragment retention; complete-line-only handoff; nonblocking candidate read; HEAD rejection; 4s stale candidate limit; 127-char request line bound; 2-second WiFiClient timeout (no 1500-second or 300-second calls); old-stream socket closure/USB lease release; WAV and PCM reads retained; unsupported probe behavior retained. **10/10 source assertions PASS.**

## Not yet verified
- **NO compilation, CI build, packaging, installer, flash or Pi deployment authorized/performed for these revisions.**
- Hardware acceptance pending: confirm new running SHA after authorized OTA, exercise fast and delayed browser/phone track switches, inspect gateway for `Connection reset by peer` and 502, verify no regressions to ordinary playback, autoplay and USB polling.
- Current running hardware remains **3.1**, source SHA `01452e660f3e8fec5d30ba84823cc0cfb5d2d58e`; source fixes are newer and **not installed**.
