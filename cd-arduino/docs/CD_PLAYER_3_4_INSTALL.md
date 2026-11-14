# LiteOn CD player 3.4 — data-only disc TOC correction

## Authorization and scope
- User explicitly responded "authorized" immediately after the #3 source-only correction checkpoint, authorizing a versioned firmware build/package for hardware testing. Installation/flashing remains a user action.
- Last hardware-confirmed running firmware: **3.3**, exact compiled source `8fe9715801497399eafacd8331a9ccf1ee1e15b4`, verified by user-supplied ESP32 status and earlier Pi OTA identity.
- Version 3.4 changes only CD table-of-contents handling needed to distinguish a *valid data-only disc* from ambiguous or failed TOC reads. It does not modify Pi code, networking, USB host drivers, WAV reader, audio transport, or read-speed/timeouts.
- Original source change `f75fb92d4f04783268f624ee80426a0ed6c251ad`, follow-up for failed-readiness reset `b1c9e9fcbcc09ed06c2949b5b63238080cff05ab`.

## Intended behavior
- A first successful, well-formed TOC containing only data tracks leaves the previous audio track list intact while awaiting confirmation.
- A second consecutive, identical successful data-only TOC clears previously advertised audio tracks, updates disc identity/revision, and reports `Data CD detected; no audio tracks`.
- A malformed TOC, failed TOC command, or failed readiness test breaks the confirmation chain without destroying a known-good track list. Successful audio TOCs restore the audio list normally.
- The Pi currently advertises `no_disc` for a CD node with **no playable audio tracks**, including a confirmed data CD; it is a protocol status, not a physical-tray-empty assertion.

## Delivery gate
- Build only by changing `cd-arduino/ci/authorized-build-request.txt` in a **marker-only** commit after the user authorization, using GitHub Actions with Arduino-ESP32 **2.0.14** and board `esp32:esp32:XIAO_ESP32S3`.
- Exact full marker-commit SHA is the source identity embedded in the compiled image, package `LiteOn CD player 3.4.zip`, and `SOURCE_MANIFEST.json`.
- Do not publish or link a guessed file or unverified binary. Record the Actions run, compilation result, SHA-256 ZIP and BIN checks, exact downloadable artifact, and installation command after verification.
- No OTA or ESP32 flash is performed by the assistant. Until user installs 3.4, the device remains on verified 3.3. Source commit and build success alone do **not** establish the stale-track issue is fixed on hardware.

## Hardware acceptance
1. Confirm the installed image reports source SHA matching the compiled manifest.
2. Begin with a playable audio CD; verify correct track enumeration.
3. Insert a known data-only disc. After two valid reads, old audio tracks should disappear and the status should indicate data-only CD. Expect a brief confirmation interval.
4. Replace with an audio CD and verify it is enumerated correctly, with changed `discRevision`.
5. Confirm one failed or malformed TOC does not wipe a working CD track list, and ordinary CD playback/Next/Previous remain functional.

**Current status:** Source-only fix staged. Compile, package, release, Pi OTA and hardware acceptance await independent evidence.

## Authorized 3.4 build and published release (2026-11-13 local / Nov 14 UTC)

**User authorization:** The user's direct "authorized" reply was taken
as approval to build/package the #3 TOC correction and provide an
installer, not as permission for the assistant to flash the S3.

- **Compiled exact source SHA:** `f7741b4c9faf160e84c8ca9fb5dde6da7989d59c`.
  Marker-only trigger was independently confirmed by GitHub commit
  contents.
- **Build and full package validation PASSED:**
  https://github.com/oolah10293/house-audio-esp32/actions/runs/43546527447
  on Arduino-ESP32 core 2.0.14 for `esp32:esp32:XIAO_ESP32S3`.
- **Verified Actions artifact:** ID `14094098007`,
  outer GitHub Actions ZIP SHA-256
  `11889e4263f1b0cb3f233acc329da2016ff4358efa992f8fbb4a148d9cbba278`.
  Independently downloaded/opened and verified the outer ZIP,
  embedded package and `SOURCE_MANIFEST.json`.
- **Firmware binary:** `HouseAudioCDInput.ino.bin`,
  800192 bytes, SHA-256
  `bd402601568a3ed0b35e8b4210fed97d0916913393d30d145ec69f5844fdab1c`.
- **Published existing verified image (no recompilation):**
  https://github.com/oolah10293/house-audio-esp32/actions/runs/43546807420
  completed success. Release
  https://github.com/oolah10293/house-audio-esp32/releases/tag/liteon-cd-3.4
  targets the same compiled SHA.
- **Release ZIP:** `LiteOn.CD.player.3.4.zip`, 547848 bytes,
  SHA-256 `ec0c620c1d24138dc534a5f9c01ff44db16cbc18d7d5550ed0729f12da50fcad`.
  The GitHub release asset's name, size and digest were independently
  confirmed through the GitHub releases API. Archive contents passed
  ZIP integrity, per-file/source-manifest, binary-hash and image-identity checks.
- **Updated Pi installer:** `cd-arduino/tools/install-liteon.sh`
  commit `9d4c456fdd92acddb8db9f79e3d8f65652faee50`; checked to
  match the published release URL and exact ZIP SHA-256.

### User-operated Pi installation

```bash
curl -fsSL https://raw.githubusercontent.com/oolah10293/house-audio-esp32/fix/liteon-usb-transport-recovery/cd-arduino/tools/install-liteon.sh | bash
```

The script downloads the pinned release ZIP, checks SHA-256, extracts
the OTA binary + manifest and runs the existing Pi->ESP32 OTA uploader,
which verifies the running `sourceSha` after reboot. The assistant
has **not** run this Pi command or flashed/deployed firmware; user
physical acceptance is still pending.

**Runtime status:** Running hardware is last independently/user-
reported as 3.3, source `8fe9715801497399eafacd8331a9ccf1ee1e15b4`.
Do not claim 3.4 installed or stale-track bug fixed on hardware
until user supplies exact OTA identity and disc-swap outcomes.
