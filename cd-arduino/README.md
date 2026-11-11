# NetworkCDProbe — Arduino IDE only

**Status: SOURCE-ONLY DRAFT. NOT COMPILED WITH ARDUINO IDE, NOT UPLOADED, NOT TESTED ON THE LITE-ON.**

This Arduino sketch replaces the earlier ESP-IDF **flashing workflow**, not the tested code base. It uses the **same USB MSC/SCSI driver** ported into an Arduino sketch library folder. No separate ESP-IDF installation, CLI, three-BIN flashing, or firmware flasher should be needed for this sketch.

Open `NetworkCDProbe.ino` in Arduino IDE. The included `src/usb_host_msc/` C files are part of this sketch; they are not separate sketches and do not need to be copied into Arduino's global libraries.

## What it attempts (first milestone only)

1. On startup, create a temporary Wi-Fi access point `CD-Probe-S3` (password `cdprobe1633`).
2. Use native ESP32-S3 USB host mode to discover the Lite-On SOHW-1633SX.
3. Read the drive identity using SCSI INQUIRY.
4. Read an inserted audio CD's table of contents and display the track numbers, type, duration and LBA.
5. Show results in a browser at **http://192.168.4.1**. Page refreshes every five seconds; a Rescan button is provided.

This is an Arduino IDE test sketch, **not yet a network music player**. It does **not** stream PCM, control the Pi, or implement Snapcast source switching.

## Arduino IDE configuration

- **Board:** XIAO_ESP32S3 (Seeed XIAO ESP32-S3).
- **Board package:** `esp32` by Espressif Systems, **3.3.12**, the version whose USB Host API was inspected. Compatibility with older versions is **not verified**.
- **Tools → USB CDC On Boot:** Disabled.
- **Tools → USB Mode:** Hardware CDC and JTAG (the sketch itself starts the IDF USB host, *not* TinyUSB).
- **Upload:** use the ordinary **Upload** button with the XIAO attached to your PC over USB-C, as with other Arduino projects.
- After flashing and disconnecting from the PC, use the XIAO's 5V/GND supply and connect its USB-C port to the drive's USB cable. Provide the Lite-On's original power separately.
- Connect a phone or PC to Wi-Fi **CD-Probe-S3**, password **cdprobe1633**; open **http://192.168.4.1**.
- The PC/phone may warn that this temporary Wi-Fi network has no internet: that's expected. It only hosts the test page.

**Note on recovery:** Since the XIAO's native USB-C port switches to host mode when the sketch runs, normal serial-port visibility can disappear. Seeed documents recovery: hold BOOT while connecting USB to the PC, then choose the bootloader COM port and use Arduino IDE Upload. This does not require a different flasher.

**Note on hardware power:** This source does not check or control USB VBUS. Verify externally powered drive, regulated XIAO 5V, shared USB ground, and isolation against backfeeding before connecting hardware.

## Verified basis (software inspection only)

- Espressif Arduino-ESP32 USB API explicitly supports using `usb/usb_host.h` directly inside a sketch; their official `USBHostIDF` example demonstrates it:
  https://github.com/espressif/arduino-esp32/tree/master/libraries/USB/examples/USBHostIDF
- Arduino-ESP32 core 3.3.11+ requires a non-null USB host `enum_filter_cb`; adapted C host driver supplies an allow-all callback if that compile-time option is enabled.
- Original optical-disc USB SCSI code: https://github.com/julpage/ESP32_CD_Player , upstream commit `04656eee5d6cfc3a6fab413a28b58a21fe299dca` (WTFPL v2).
- Earlier `cd-source/` ESP-IDF probe was compiled successfully under ESP-IDF v5.2.8 but that does **not** establish Arduino sketch compatibility.

## Outstanding validation

- Arduino IDE compile and link.
- Confirm S3 USB Host stack is available in selected board package.
- Confirm the Lite-On USB device enumeration and SCSI command compatibility.
- Confirm TOC durations and disc insertion/removal reporting.
- Confirm diagnostics web page and rescan work with USB host active.

No firmware build, package, or release is authorized by this source-only change.
