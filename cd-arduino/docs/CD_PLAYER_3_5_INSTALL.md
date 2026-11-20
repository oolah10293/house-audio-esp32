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

## Verified build and release — 2026-11-20

- Compiled source: `6359c1e0111f4f4ddcc4ecab0f3d41d0825d2fab` on main; Arduino ESP32 2.0.14, XIAO ESP32-S3.
- [CI run 44771766407](https://github.com/oolah10293/house-audio-esp32/actions/runs/44771766407) completed successfully: compile, exact source/ZIP verification, artifact upload and publication all passed.
- Sketch: 800197 bytes / 1310720; static global memory 66068 / 327680 bytes.
- [Published ZIP](https://github.com/oolah10293/house-audio-esp32/releases/download/liteon-cd-3.5/LiteOn.CD.player.3.5.zip): 547648 bytes; SHA-256 `607dafcacf676bc3b173e750e7b0b6f9392a648ccf9b8c8b16270b9d7240c115` (confirmed against release asset metadata).
- The main-branch installer now pins that exact URL and digest. On the Pi after stopping CD playback: `curl -fsSL https://raw.githubusercontent.com/oolah10293/house-audio-esp32/main/cd-arduino/tools/install-liteon.sh | bash`. Old 3.4 installer remains available from `6359c1e0111f4f4ddcc4ecab0f3d41d0825d2fab` at the same path for rollback.
- Hardware installation, runtime sleep setting, actual improvement in radio utilization/retries and listening acceptance remain unverified. No Pi/gateway deployment performed.

## Hardware OTA identity acceptance — 2026-11-20

User supplied Pi terminal output from the pinned 3.5 installer. Package checksum verification passed. Before update, the node reported version 3.4, source f7741b4c9faf160e84c8ca9fb5dde6da7989d59c and MD5 c6c556a7d44df7cf519224e5b396ca63. Uploaded application: 800560 bytes, SHA-256 69726d51105d4c74254224eddaa6365daf1a67278e4a02265932cd27641950b4. OTA accepted and after reboot the updater reported exact expected source 6359c1e0111f4f4ddcc4ecab0f3d41d0825d2fab, running MD5 8d222896ade70fde57dbaf1da7a46069.

**PASS — installed-image identity by user-supplied output.** Runtime wifiPowerSave diagnostic and network/playback performance are not yet verified. Next: curl -s http://192.168.1.175/api/firmware; capture actual power-save status before comparing playback. No additional firmware build or deployment performed.

## Runtime Wi-Fi power-save acceptance — 2026-11-20

User supplied `curl -s http://192.168.1.175/api/firmware` output: version 3.5, sourceSha 6359c1e0111f4f4ddcc4ecab0f3d41d0825d2fab, sketchMd5 8d222896ade70fde57dbaf1da7a46069, freeSketchSpace 1310720, wifiPowerSave disabled, bootSession 242327f09cf59451. **PASS: actual driver power-save state reports disabled.** Playback performance, airtime, retries and functional CD regression acceptance remain pending. Compare CD playback with six listening nodes and unchanged AP assignments. Documentation only; no new build or deployment.

## Six-node CD playback acceptance — 2026-11-20

After installing Lite-On 3.5 and verifying that firmware reports `wifiPowerSave: disabled`, the user reported: **"i've listened to most of a cd and no dropouts with 6 speakers and the cd player s3 connectted"**. This is a **PASS by user observation** for nearly one complete audio CD with six listening/rendering nodes plus the Lite-On ESP32-S3 streaming source simultaneously active; no audible dropouts were reported during that session.

This does **not** prove that the Wi-Fi power-save change caused the improvement, does not constitute a controlled A/B test, and does not establish zero retries, lower channel utilization, or indefinite reliability. No new AP/channel configuration, additional diagnostics, or device source hashes were supplied for the test. Leave the current setup unchanged and monitor ordinary listening. No firmware source, build, or deployment change.

## AP utilization during accepted listening session — 2026-11-20

User reported Basement AP 25% utilization and Guest Room AP 31% utilization, after previously reporting nearly an entire CD without audible dropouts on six listening nodes plus the CD S3 with firmware 3.5. Values are user observations, not independently polled readings. The Lite-On had previously been moved from Basement (channel 6) to Guest Room (channel 11); no identical AP-assignment pre/post control has been established. Earlier Basement CD playback reached approximately 47%, but **do not attribute the difference to Wi-Fi power-save changes** because AP placement/load differed. No repeat packet retry sample has been taken. Maintain current settings pending ordinary listening. Documentation-only checkpoint.
