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
