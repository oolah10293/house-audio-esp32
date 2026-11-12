# Arduino-IDE migration checkpoint (2026-11-11)

**Context:** User requires Arduino IDE for compiling and uploading firmware. Prior compilation with standalone ESP-IDF and delivery as separate .bin files caused needless friction and are no longer the selected delivery workflow.

**Verified:**
- Seeed XIAO ESP32-S3 supports native USB OTG host.
- Espressif Arduino-ESP32 provides an **IDF USB Host** API usable from Arduino sketches, demonstrated in official `USBHostIDF` example. Core version 3.3.11+ requires enumeration filter callback.
- Original `cd-source/` source includes SCSI INQUIRY and CD READ TOC, and compiled under ESP-IDF v5.2.8. This does not prove Arduino compatibility.
- New Arduino sketch is independent of the user's current renderer firmware and does not touch House Music Pi.

**Source additions:** `cd-arduino/NetworkCDProbe/NetworkCDProbe.ino` and embedded USB/SCSI C code in `src/usb_host_msc/`, plus readme. It offers local AP status to avoid an additional UART adapter when USB-C is attached to drive.

**Not verified:** Arduino compile, upload, USB enumeration, Lite-On compatibility, Wi-Fi AP operation, actual track list.

**Build permission:** No new build or package has been authorized for this Arduino-specific rewrite. Don't assemble a binary or claim a pass. Next milestone is compiling through the Arduino IDE workflow when the user requests it; then USB hardware test.

**Release/deployment permission:** None.

## User test: Arduino IDE compilation succeeded; flashing stalled (2026-11-11)

User-reported output, not independently reproduced by us:
- Arduino IDE compiled `NetworkCDProbe.ino` successfully: 746729 bytes program (56%), 67216 bytes globals (20%). This verifies compilation and linking in the user's installed board package, but installed core version has not been independently identified.
- First upload to ESP32-S3 COM25: chip identified, stub uploaded and ran; failure after switching to 921600 baud: `No serial data received`.
- User retried at reduced upload speed (115200 recommended); chip identified, stub ran, but failed during flash-chip verification: `Unable to verify flash chip connection (No serial data received.)`. Thus the baud-rate hypothesis is not sufficient.
- Firmware **not flashed**; no Lite-On hardware operation verified.

Recommended next diagnostic (not yet reported as done): Disconnect peripherals/external 5V, leave only PC USB, force Seeed XIAO ESP32-S3 ROM bootloader with BOOT button held during USB connection, select resulting Arduino IDE port, and retry normal Upload at 115200. Seeed documentation: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ . Escalate power/cable/reset if reproducible.
