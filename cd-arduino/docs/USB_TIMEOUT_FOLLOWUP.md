# Lite-On USB transport timeout — still provisional

**OPEN / NOT YET HARDWARE-OPTIMIZED — checkpoint 2026-11-13 after ESP32 3.3 acceptance and Pi track-switch repair.**

## What is actually in the running source

The recovery branch `fix/liteon-usb-transport-recovery` increased select
USB Mass Storage Bulk-Only Transport timeouts from **200 ms to 1,500 ms**
(7.5×). In `cd-arduino/HouseAudioCDInput/src/usb_host_msc/usbhost_msc_cmd.c`:

- **CBW**: 1,500 ms maximum wait when sending the command block wrapper
  (`usbhost_bulkTransfer(... HOST_TO_DEV, 1500)`).
- **CSW after a nonzero data stage**: 1,500 ms maximum wait to read
  the command-status wrapper.
- **Stalled-CSW retry**: 1,500 ms maximum wait after clearing endpoint
  HALT.
- **Other waits are not globally 1,500 ms**: the data stage and
  no-data-command CSW use caller-supplied `timeout`; unrelated USB control
  transfer waits are unchanged. The `usbhost_bulkTransfer` argument is
  in **milliseconds**, passed to `xfer->timeout_ms` by
  `usbhost_driver.c` and used with `pdMS_TO_TICKS`.

**This is a timeout ceiling, not a fixed 1.5-second delay.** Normally
completed transfers need not wait for the ceiling. The first-audio
latency measured in the Pi network captures (approximately 373 ms
and 705 ms from WAV header to first PCM, on two different track
connections) is NOT a measure of USB CBW or CSW transfer duration.
Likewise the earlier roughly 2-second House Music MPD `play N` timeout
was on the Pi control path, not proof of a USB transport timeout.

## Updated field status

- The user installed **ESP32 CD firmware 3.3**, exact compiled source
  `8fe9715801497399eafacd8331a9ccf1ee1e15b4`, via Pi OTA,
  and confirmed the CD played and track selections worked much better.
- The remaining GUI `/inputs/play` 503 timeouts were traced on
  the Pi to direct MPD `play N` while already playing another CD
  track. Back-to-back MPD `stop; play` was measured at roughly
  **4 ms + 4 ms**. Following a small Pi server-only stop-before-play
  fix, the user supplied **four consecutive HTTP 200** responses from
  normal CD track selections. This is evidence of a Pi-side control fix,
  not evidence for the USB timeout value.
- Neither the 3.3 installation nor the Pi fix isolated the effects of
  the USB timeout increase versus concurrent USB transport recovery
  changes. No transfer-stage latency distribution or CBW/CSW timeout
  counts on the physical Lite-On have been gathered.
- Therefore **1,500 ms remains a diagnostic/provisional ceiling**, not
  an accepted minimum or demonstrated requirement.

## Focused follow-up, before any firmware change

1. Record CBW, data, and CSW phase durations and whether a transfer
   approaches 200 ms or actually times out. Distinguish legitimate
   slow drive response from USB scheduling delays and recovery events.
2. With a known-good baseline, test a *single timeout variable change*
   (e.g. 500 ms) **only after the user explicitly authorizes a firmware
   build and flash**. Preserve a rollback image and the working Pi
   configuration; don't bundle unrelated network/CD changes.
3. Verify power-up/automatic disc detection, different-disc insertion,
   initial PCM startup, sustained whole-track streaming, repeated track
   switching, and recovery after transient USB errors. Do not declare a
   timeout optimal from one successful track.
4. **Acceptance:** shortest empirically reliable timeout with successful
   CD detection, streaming, and recovery, backed by actual hardware
   observations.

Do **not** compile, build, package, release, deploy, or flash firmware
on the strength of this documentation update.
