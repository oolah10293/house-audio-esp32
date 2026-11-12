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

## Arduino core/version clarification and next isolation test (2026-11-11)

User confirmed their Arduino IDE uses **esp32 by Espressif Systems 2.0.14**, intentionally selected previously for compatibility with a separate ESP32 composite-video CRT visualizer. **Do not direct them to update this shared Arduino core without a plan to preserve the working visualizer**.

The CD probe sketch **compiled successfully** under their current 2.0.14 installation, based on user-provided output. The prior README's version 3.3.12 requirement is a code-inspection target, *not* a demonstrated minimum for compilation. Arduino ESP32 2.0.14 runtime / USB host compatibility is still unverified; do not assume a successful compile proves hardware behavior.

The separate failure is during flashing: both 921600 and 115200 uploads reached `Stub running...` and then lost serial communication (before completion). Next requested diagnostic is a **normal Arduino IDE built-in Blink upload to the same XIAO under unchanged board/port settings**, to distinguish a generalized Arduino upload problem from a CD sketch-specific one. Blink result pending; no hardware or firmware claim yet.

## Blink isolation result and precise board-setting lead (2026-11-11)

User-reported: Arduino IDE built-in Blink sketch compiles (249445 bytes) but fails on COM25 with **exactly the same** esptool.py v4.5.1 sequence as CD probe: "Uploading stub... Running stub... Stub running..." followed by "Unable to verify flash chip connection (No serial data received.)". User repeated this several times. **The failure is independent of CD sketch code.** Do not keep modifying CD source to solve this Arduino flashing problem.

A very close published match: Seeed Studio forum user on XIAO ESP32-S3 Sense encountered the exact error while uploading **Blink**, and reported success after selecting **OPI PSRAM** and **DIO 80MHz flash mode** in Arduino Tools: https://forum.seeedstudio.com/t/xaio-esp32s3-sense-unable-to-use-with-arduino-2-x-ide/271050/9 . This is someone else's report, not verified on the user's exact XIAO hardware. Proposed next test: Check Arduino Tools > Flash Mode set to DIO 80MHz and Tools > PSRAM set to OPI PSRAM, *if these exact menu choices are offered*, then retry Blink; if already selected, do not assume the lead is applicable. No changes to source, core version (must remain 2.0.14 for CRT visualizer), cabling or XIAO boot/reset buttons.

## Correct XIAO board selected: upload retry still fails (2026-11-11)

User selected the dedicated `XIAO_ESP32S3` board in Arduino IDE 2.0.14 (rather than generic `ESP32S3 Dev Module`). User's latest log: sketch 733225 bytes (21%), maximum 3342336 bytes; esptool.py 4.5.1 on COM25 detects ESP32-S3 revision 0.2; uploads and starts stub; fails immediately following `Changing baud rate to 921600` with `No serial data received`. This confirms board settings/partition configuration changed, **not that the correct board definition fixed serial uploads**. It also indicates upload speed reverted to 921600 with changed board selection. Next isolated test: set Tools > Upload Speed > 115200 under the **XIAO board selection**, retry known compiling Blink, and keep all other options unchanged. Note prior 115200 retry occurred while the generic Dev Module board was selected, so is not the same configuration.

## Confirmed repeat at 115200 with dedicated XIAO board; USB ROM loader workaround proposal (2026-11-11)

User log: `XIAO_ESP32S3`, Arduino core 2.0.14, esptool v4.5.1, COM25, sketch compiled 733225 bytes (21%), chip identified, `Stub running...` followed by `Unable to verify flash chip connection (No serial data received.)`. This occurred **at 115200 baud**, so upload speed and selecting dedicated XIAO board have not solved the problem.

A published Arduino Forum ESP32-S3 report with **the same exact message** describes resolution using esptool `--no-stub`, which stays with the ESP32 ROM loader rather than transferring/running a stub: https://forum.arduino.cc/t/esp32-s3-reset-required/1172767/2 and official Espressif docs https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/advanced-options.html . Workaround **not verified on user's board**.

A reversible *Arduino IDE Upload-button* path is supported by Arduino's `platform.local.txt` override: place a `platform.local.txt` alongside installed 2.0.14 `platform.txt`, containing `tools.esptool_py.upload.pattern="{path}/{cmd}" --no-stub {upload.pattern_args}`; restart IDE; retry Blink first. This upload recipe was independently inspected in the upstream 2.0.14 tag. Modifies upload behavior for **all boards under the 2.0.14 ESP32 package**, so remove override following diagnosis/when not needed to preserve CRT visualizer setup. Neither workaround nor hardware result tested yet.
