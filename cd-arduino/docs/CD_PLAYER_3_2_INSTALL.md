# LiteOn CD player 3.2 — stream handoff correction

## Source baseline
- User-installed 3.1 verified from Pi OTA on 2026-11-13: running commit `01452e660f3e8fec5d30ba84823cc0cfb5d2d58e`, running MD5 `741945608bf41301f27a87be68ee83fb`.
- Pi gateway logged HTTP 502 and `Connection reset by peer` on attempted new tracks 10 and 6 while earlier tracks closed with `Broken pipe`. This proves the failing upstream connection but not the precise root cause.
- The Arduino-ESP32 2.0.14 `WiFiClient::setTimeout` API uses seconds. The old transport had `setTimeout(300)` and `setTimeout(1500)`; those values were mistakenly treated as milliseconds.
- Source-only corrections committed before this authorized build: `f2bcd8d9a520a11f15956222888cbaa04a20d657` (nonblocking complete-request handoff) and `132638bb77e1457c076bde759e8acaa6f578779b` (2-second client timeout).
- Detailed root-cause analysis: `CD_STREAM_HANDOFF_CHECKPOINT_2026-11-13.md`.

## What changes in 3.2
- Firmware `CD_RELEASE_VERSION` becomes `3.2`; `CD_SOURCE_SHA` is generated from the exact CI checkout commit.
- New incoming track connections retain incomplete request lines across CD audio blocks, and only replace the stream after a complete valid GET request line arrives.
- A stale partial request expires after four seconds; unsupported HEAD requests cannot preempt playing audio.
- Incoming HTTP request/header timeout is bounded at two seconds (WiFiClient units); no separate USB CBW/CSW timeout changes.
- No changes to CD/SCSI commands, Pi gateway, CD audio PCM layout, Wi-Fi credential handling, registration, or the OTA API.

## Delivery
A user-authorized GitHub Actions compile/build from one commit on the pinned `esp32:esp32@2.0.14` toolchain and `XIAO_ESP32S3` board. The package includes a compiled OTA image, SHA-256 manifest, Pi uploader and matching source snapshot.

Preferred update method (run only on the Raspberry Pi):
```sh
curl -fsSL https://raw.githubusercontent.com/oolah10293/house-audio-esp32/fix/liteon-usb-transport-recovery/cd-arduino/tools/install-liteon.sh | bash
```

This installer must be updated to a 3.2 release URL and verified archive SHA-256 after the actual 3.2 build, and the release asset must be published and verified before delivery. No Windows file transfer, manual ZIP extraction, Arduino IDE, or USB flashing should be needed for this already-OTA-capable S3.

## Verification and hardware acceptance
**Build status pending until CI completes; hardware 3.2 status pending until user runs Pi OTA and provides the observed new SHA.** A compile PASS is not evidence the track-switch failure is fixed. Compare rapid sequential track changes and track 1 -> 2, 4 -> 10, 11 -> 6 using gateway journal at `house-audio-input-gateway.service`, listening for 502 / reset and checking actual audio. The existing working 3.1 firmware remains on the S3 until the user installs 3.2.

## Authorization
User explicitly stated: “this is your explicit authorized 3.2 build” on 2026-11-13. This authorizes the 3.2 firmware build/release; not arbitrary future versions or installing on user hardware.

## Verified release checkpoint — 2026-11-13
- **Authorization:** User explicitly said “this is your explicit authorized 3.2 build” in this conversation.
- **Compiled exact Git commit:** `0fc770eb88753e5ef8a523c530f5b5741028f5aa`.
- **Build CI:** https://github.com/oolah10293/house-audio-esp32/actions/runs/43484170374 — SUCCESS; all steps, pinned ESP32 2.0.14 compile, manifest/source checks, and artifact upload PASS.
- **Published release CI:** https://github.com/oolah10293/house-audio-esp32/actions/runs/43484595830 — SUCCESS; approval marker and verified existing binary release, no second compile.
- **GitHub release:** https://github.com/oolah10293/house-audio-esp32/releases/tag/liteon-cd-3.2 .
- **Published artifact:** `LiteOn.CD.player.3.2.zip`, 545,826 bytes. Publisher and local inspection confirmed SHA-256 `8086352210cec9735b6e2c289f361abc683ce4400058d0a632a058568c1a4599`; the download asset URL is byte-for-byte the URL specified by `cd-arduino/tools/install-liteon.sh`, independently confirmed by the GitHub release API.
- **OTA application binary:** `HouseAudioCDInput.ino.bin`, 798,912 bytes; SHA-256 `3f4120535bf18ea4f4a64bd6e2671f5a2fdcf16666aefdc9925e3b63978690b3`.
- **Running firmware status:** 3.1 previously verified on user hardware. 3.2 **not yet flashed/tested**; do not claim timing fix without track-switching test and gateway journal.

## User-confirmed OTA installation of 3.2 — 2026-11-13
- User ran the one-command Pi installer `cd-arduino/tools/install-liteon.sh` from `ads-suck@pihole`.
- Pi updater read existing installed firmware **3.1**, source SHA `01452e660f3e8fec5d30ba84823cc0cfb5d2d58e`, running MD5 `741945608bf41301f27a87be68ee83fb`.
- It verified the published archive, uploaded the **798,912-byte** firmware with SHA-256 `3f4120535bf18ea4f4a64bd6e2671f5a2fdcf16666aefdc9925e3b63978690b3`, received `Image accepted`, and polled after reboot.
- **OTA installation PASS on actual hardware:** `PASS: ESP32 reports expected running commit: 0fc770eb88753e5ef8a523c530f5b5741028f5aa`.
- New running image MD5: `28fc4aea5e52d4781af59dbdd016d624`.
- **Not yet verified:** whether the 3.2 stream handoff fixes gateway 502s, MPD track skipping, or CD audio dropout. Test with rapid/sequential track changes against `house-audio-input-gateway.service` journal before claiming behavioral correction.

## Hardware track-switch acceptance — FAIL on 3.2 (2026-11-13)
- From user's live Pi journal while running installed 3.2:
  - `13:42:28`: gateway GET `/audio/cd-7452ed8dbd44/10.wav` reported HTTP 200.
  - `13:42:33`: Track 10 stream failed with `[Errno 32] Broken pipe`; `missing=38811416` bytes.
  - `13:42:33`: Track 4 failed **before WAV headers** with `[Errno 104] Connection reset by peer`, gateway HTTP **502**.
  - `13:42:33`: Track 5 GET returned HTTP 200.
- Browser screenshot independently displayed the MPD error `Failed to decode .../4.wav; got HTTP status 502` while now playing Track 5, confirming a skip.
- **Definite result:** 3.2 does not resolve this failure. Do not claim the source timeout corrections fixed track switching, despite successful build and OTA install. Preserve as failed hardware acceptance.
- The Python gateway source `house-audio-server` branch `feature/cd-auto-takeover` / `pi_input/gateway.py` matches the user's pasted proxy code around lines 145-212 and uses `ThreadingHTTPServer` to proxy overlapping requests to S3.
- Review of S3 `HouseAudioInputTransport.h` confirms **single-stream task** and unconditional **replacement on every new GET /track/**. This can conflict if MPD opens another track to prefetch or prepare it while the chosen stream is active. MPD documentation says its optional input cache prefetches queued songs, but whether it is active on the user's Pi or caused this failure is **unverified**.
- Gateway access logs are emitted at HTTP response time rather than connection arrival time. The same-second 502 for 4 followed by HTTP 200 for 5 is not proof that track 5 was opened first or that MPD prefetch is the source of the reset.
- Next diagnostic should preserve **wire-level GET arrival order and TCP RST direction** around one track transition; do not iterate with another blind firmware build. No 3.3 build/package/deployment authorized.

## TCP direction checkpoint — 2026-11-13 13:47:50 EST
- User ran `tcpdump` on Pi filtering S3 (`192.168.1.175:8080`) HTTP GET and TCP RST. The interface's promiscuous-mode warning was non-fatal.
- At `13:47:50.750776` Pi `192.168.1.12:40298` sent the complete HTTP request `GET /track/4.wav HTTP/1.1` with `Connection: close` to S3.
- `13:47:50.760549`–`13:47:50.762809`: Pi transmitted multiple RST segments on **another existing source port 52460**, consistent with canceling an earlier stream.
- At `13:47:50.786714` **S3 at 192.168.1.175:8080 transmitted TCP RST/ACK to Pi port 40298**. It is the newly requested Track 4 socket, and the reset arrived ~35.9 ms after the observed GET.
- At `13:47:50.792026` Pi then sent `GET /track/5.wav` on *another* socket (40312).
- This confirms **direction of the new-track reset: S3 -> Pi**, independently of Pi canceling the old socket. Track 5's GET appears **after** the RST of Track 4 in this capture: it cannot be the direct cause of that already-transmitted reset. This weakens any claim that Track 5 prefetch interrupted Track 4, while not excluding another concurrent connection not visible under the narrow GET/RST filter.
- The capture filter did **not** include SYN/FIN or HTTP response headers; it cannot establish whether S3 sent an HTTP status line before the reset, whether any other socket was accepted, or which firmware branch closed Track 4. The source under investigation remains `HouseAudioInputTransport.h`; Arduino-ESP32 2.0.14 `WiFiClient` and `WiFiServer` were inspected, including shared socket-copy ownership.
- Next diagnostic: one short `tcpdump` capture that includes SYN, FIN, RST, GET and `HTTP` responses; avoid another speculative firmware build. User has **not** authorized any 3.3 build.

## Second TCP capture — 13:51:58 EST, 2026-11-13
- Pi sent SYN for Track 4, source port 55490 at 13:51:58.571225; S3 SYN/ACK at 13:51:58.575026; Pi sent complete `GET /track/4.wav` with `Connection: close` at 13:51:58.575071.
- Pi separately FIN/RST old stream, source port 43104 at 13:51:58.572695/574136; then S3 emitted an old-stream `FP.` packet with 296 bytes at 13:51:58.591222, which Pi reset again. Thus old and new streams overlapped during closure.
- At 13:51:58.612871, **S3 sent RST/ACK to Pi port 55490 (Track 4), 37.8 ms after the GET, without an observed HTTP response**.
- Pi initiated Track 5 on a different connection (55504) afterward; S3 returned `HTTP/1.1 200 OK` at 13:51:58.635946.
- **Verified:** the S3 side drops the newly requested Track 4 connection. It is not an 8s gateway timeout, and the later Track 5 request cannot have triggered this earlier RST.
- **Source-review finding:** `audioStreamTask` checks `audioCheckReplacement(candidate,...)` only inside its old track PCM read loop. If the active stream terminates via a failed `audioWriteAll` while an accepted candidate request is still incomplete, its next outer loop iteration takes `pending` or `audioServer.available()` and **does not service the saved candidate**. This is a real potential candidate socket lifetime defect; it is **not yet proven** that it triggered this specific 37.8 ms reset.
- Another candidate failure path involves a just-accepted `WiFiClient` failing `operator bool()`: Arduino-ESP32 2.0.14 `connected()` checks errno after zero-length nonblocking recv. Whether that path occurs on this user's S3 has not been measured.
- Next steps require precise instrumentation of accepted/rejected/closed connection paths or a reviewed source-only candidate ownership correction with targeted regression checks. **No 3.3 build, package, deployment or installation has been authorized.**

## Comparison with original CD project and upstream Arduino networking fix — 2026-11-13
- The original USB CDDA source is `julpage/ESP32_CD_Player` (main commit `04656eee5d6cfc3a6fab413a28b58a21fe299dca`). The repo plays CD audio **locally through I2S**. In `main/cdPlayer.c`, `cdplayer_task_playControl` changes a track index/read-frame count on Next/Previous, reads sectors with `usbhost_scsi_readCD`, and fills I2S buffers. **No HTTP server, WiFi WAV stream replacement or TCP handoff exists in that original project**. Its USB/SCSI implementation was previously ported into our Arduino firmware; the failing HTTP handoff is our adaptation, not an upstream feature.
- A more directly applicable **verified source-level issue** is in `espressif/arduino-esp32` core 2.0.14: `libraries/WiFi/src/WiFiClient.cpp`, `WiFiClient::connected()` calls `recv(fd(), &dummy, 0, MSG_DONTWAIT)` and checks `errno` when `res <= 0`. The zero-length call may return 0 without changing errno. A **stale ECONNRESET/EPIPE** from earlier socket activity could then make a healthy new client report disconnected; destructing/dropping its last WiFiClient reference closes the socket without HTTP response.
- Espressif issue https://github.com/espressif/arduino-esp32/issues/10559 (Nov 3, 2024) explicitly reports this zero-length receive / stale errno error in `NetworkClient::connected()`. Their merged fix https://github.com/espressif/arduino-esp32/pull/10640 (Nov 25, 2024) replaced the zero-length receive with a `MSG_PEEK` read to prevent interpreting an active socket as closed. Our pinned **2.0.14 core predates this fix**. This is an independently documented library defect, not just speculation about the sketch.
- Our `HouseAudioInputTransport.h` uses `WiFiClient client = pending ? pending : audioServer.available(); if (!client) ...` and `if (!candidate)` in replacement handling. `operator bool()` calls the affected `connected()` implementation. That provides a credible exact mechanism for S3 silently resetting Track 4 within 38 ms while the prior socket is being closed. **Still not experimentally proven** to be the specific trigger; test with a narrow source-only change or improved per-socket tracing. Avoid unrequested toolchain upgrades because other projects intentionally use 2.0.14.
- Additional discovered upstream issue https://github.com/espressif/arduino-esp32/issues/13059 discusses distinct EOF / socket lifetime bugs in newer 3.x core, not direct evidence for this 2.0.14 RST, and should not be conflated with issue #10559.
- No new firmware code/build/package/flash/deployment performed during this research; prior 3.2 hardware RST failure remains the authoritative acceptance result. User's previously expressed permission for updated firmware was paused by their request to research existing GitHub work first; reconfirm scope before initiating a new release if ambiguous.
