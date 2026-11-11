# Network CD source — USB/TOC probe (source-only)

**Status: source checkpoint, NOT BUILT, NOT FLASHED, NOT TESTED ON HARDWARE.**

This is milestone 1 for a future ESP32-S3 network audio-CD source for House Music.
It is deliberately isolated under `cd-source/` on branch `feature/cdda-usb-probe`.
No existing Snapcast renderer, Raspberry Pi service, MPD setting, or Home Audio YAML is changed.

## Hardware being used
- Seeed Studio XIAO ESP32-S3 (native USB-OTG host).
- Lite-On SOHW-1633SX external USB DVD/CD drive with its own power supply.
- XIAO supplied by a separately regulated 5 V input.
- USB-C connected to Lite-On through a USB host adapter.
- Optional USB-to-UART adapter for logs: XIAO D6/GPIO43 UART0 TX → adapter RX; XIAO D7/GPIO44 UART0 RX → adapter TX; common GND. **3.3 V logic only, not RS-232 voltage.**

Seeed's 5V pin is connected to VBUS; Seeed documents a diode for external 5V supply. This project has **not electrically verified** that the external supply and the Lite-On's USB cable are isolated, or that valid USB VBUS is present. Confirm power/ground before connecting; do not attach a PC USB supply and independent 5V supply together without proper isolation.

## What the current source attempts
1. Start ESP-IDF USB host services.
2. Identify the USB Mass Storage/Bulk-Only device.
3. Issue SCSI INQUIRY and print the vendor/model.
4. Poll for a ready disc.
5. Read the CD table of contents (TOC) and list tracks, sector addresses and durations, distinguishing audio/data tracks.
6. Handle loss of disc / device with status messages.

The program does **not** read audio sectors, stream PCM, speak Snapcast, control MPD, use Wi-Fi, or install on any device.

## Provenance and licensing
The `components/usb_host_msc` layer is adapted from
[julpage/ESP32_CD_Player](https://github.com/julpage/ESP32_CD_Player),
upstream commit `04656eee5d6cfc3a6fab413a28b58a21fe299dca`.
That upstream repository uses the WTFPL version 2 license; a copy is in `THIRD_PARTY_LICENCE`.
It provides USB-host enumeration and SCSI MMC operations including `READ TOC` and `READ CD`.
The source here intentionally does **not** include upstream TFT, OLED, button or I2S playback code.

## After explicit authorization to build
Working source root will be `cd-source/` (ESP-IDF 5.2.x baseline, target `esp32s3`).
First verify USB connectivity and UART logs with a known-good audio CD.
Do **not** assume this drive supports a USB full-speed fallback until hardware proves it.

**Acceptance:** logs contain the Lite-On's SCSI INQUIRY identity and the correct track count and order for a known audio CD. If USB enumeration, command transport, or TOC fails, capture the UART log and fix that exact problem before adding streaming.

## Why a separate input architecture
Per `house-audio-server/docs/SESSION_BEHAVIOR.md`, the Raspberry Pi owns MPD queue/control and Snapserver distributes synchronized playback. A CD reader located elsewhere is a **source**, not a passive Snapcast output.
Only after the USB milestone passes will we design the Wi-Fi transport and controlled Pi-side input without breaking house sessions.

## Known source risks
- This is unbuilt firmware; API compatibility has **not** been compiler-checked.
- The upstream USB transport was developed against ESP-IDF 5.2.1, not the latest installed toolchain.
- The driver assumes a Mass Storage/Bulk-Only interface; the specific Lite-On has not been enumerated.
- Handling surprise unplug mid-transfer and USB BOT recovery may need hardening.
- The XIAO USB-C port cannot also serve as its normal USB programming/debug connection while occupied by the drive.
- The disc probe currently recognizes media changes by detecting a not-ready interval. Rapid swaps may require an explicit media-change event check.
