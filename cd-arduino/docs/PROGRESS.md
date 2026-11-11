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
