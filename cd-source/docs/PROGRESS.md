# Source checkpoint — 2026-11-11

**Authorized:** source development for the USB CD recognition and track-list milestone.
**Not authorized:** compiling/building, packaging, flashing, deployment, modifying running Pi services.

## Verified before editing
- `oolah10293/house-audio-esp32` main head: `c539bf70b7f925e30a32eee09357f30bf4d50299`.
- `oolah10293/house-audio-server` main includes MPD -> FIFO -> Snapserver for the house stream.
- Upstream `julpage/ESP32_CD_Player` source targets `esp32s3` (ESP-IDF 5.2.1), includes CDDA SCSI commands, WTFPL v2 license.
- Seeed XIAO ESP32-S3 has native USB OTG; UART0 is D6/GPIO43 TX, D7/GPIO44 RX.

## Source changes in this checkpoint
- Isolated `cd-source/` ESP-IDF project.
- Vendored minimal USB Mass Storage/BOT + MMC command layer.
- Added independent UART-only CD identity and TOC probe.
- No renderer modification and no Pi-side integration.

## Validation status
- Documentation and source inspected.
- **Compile: PASS (GitHub Actions, see below).** Binary generated in CI only.
- **No hardware test, flashing, release packaging, or deployment.**
- Next action: obtain separate authorization for a firmware package and a device flash/test before attempting the Lite-On compatibility check.
- Hardware proof remains outstanding; do not call the project working until real Lite-On logs show INQUIRY and correct CD TOC.

## Compile checkpoint — 2026-11-11

**Authorization:** The owner explicitly said "proceed with the build." This authorized compiling, not flashing, deploying, or distributing a firmware release.

- GitHub Actions build: https://github.com/oolah10293/house-audio-esp32/actions/runs/43114639459
- Source built: `82f71457485307702f16a18eb8d2134fa0aeeeab`
- ESP-IDF: `v5.2.8`; target `esp32s3`.
- **RESULT: PASS.** `idf.py build` completed, and application, bootloader and partition binaries existed and were nonempty in the runner.
- Application: `xiao_usb_cdda_probe.bin` = `0x3bf80` bytes; SHA-256 `1e9196dd959bbe65cf91431a005babda859f07a3f75264d6c303f673d5a61cce`.
- First CI run failed because `main/CMakeLists.txt` did not declare `usb_host_msc` in `REQUIRES`. Fixed in commit `82f7145`; second CI run passed.
- **NO DEVICE PROOF.** USB enumeration, Lite-On SCSI command compatibility, physical power/VBUS arrangement and CD track listing remain untested.
- **NO RELEASE.** CI built firmware only for verification; it did not upload a package, flash the device or deploy Pi changes.

