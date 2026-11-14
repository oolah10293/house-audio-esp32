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
