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
- **No code compilation. No binary. No hardware test.**
- First next action upon explicit build permission: compile **only** this probe, then separately request permission to flash to the target XIAO.
- Hardware proof remains outstanding; do not call the project working until real Lite-On logs show INQUIRY and correct CD TOC.
