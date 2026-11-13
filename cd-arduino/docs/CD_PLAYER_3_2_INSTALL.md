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
