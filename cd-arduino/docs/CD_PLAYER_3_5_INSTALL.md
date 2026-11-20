# LiteOn CD player 3.5 — Wi-Fi power-save isolation

User explicitly authorized firmware ready to install on 2026-11-20. Start from main a3c51ec6c107df3dca24aa705d2c6345ff6a39fe; installed 3.4 image source f7741b4c9faf160e84c8ca9fb5dde6da7989d59c retained for rollback.

## Change
- Disable Wi-Fi modem sleep in station mode before WiFi.begin using Arduino ESP32 2.0.14 WiFi.setSleep(false). Arduino applies the stored setting on station start. Log failure.
- /api/firmware reports wifiPowerSave disabled/enabled/unknown from esp_wifi_get_ps, not an assumed successful setting.
- No changes to USB/SCSI, CD track reads, PCM WAV, transmission pacing, Pi gateway, renderer firmware or network credentials. No FLAC encoding in this isolated release; investigate separately.
- The working hypothesis is reduced Wi-Fi latency/jitter, not guaranteed lower channel utilization or resolution of dropouts. Modem sleep disabling does not reduce audio payload size.

## Installation and acceptance
Use the existing Pi OTA helper with verified 3.5 package manifest. Do not flash from assistant. Stop playback before installation. Verify reported version/source SHA and wifiPowerSave=disabled at http://192.168.1.175/api/firmware, then compare CD playback with the same AP and active speakers, Omada channel use/retries and audible dropouts. Check track change and disc swap. Roll back to existing 3.4 release if needed. No Pi server changes required.

## Status
Source checkpoint; build/ZIP verification and hardware acceptance recorded separately when actually completed. No claim of improved Wi-Fi performance until field measurements.
