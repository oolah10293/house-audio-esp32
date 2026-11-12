# Reusable audio input build checkpoint — 2026-11-11

**Authorized:** User explicitly authorized implementing the House Music input-node build plan.

**Working baseline, not replaced:** The Arduino IDE NetworkCDProbe was user-verified on the XIAO ESP32-S3 with Lite-On audio CD: Wi-Fi status page shows 11 audio tracks, zero data tracks and LBAs. Original probe source is preserved.

**New source:** HouseAudioCDInput Arduino sketch, separate generic PCM/WAV transport header, and a copy of the existing proven USB MSC/SCSI C driver. Uses Arduino ESP32 2.0.14 and the user-tested --no-stub upload override. Nodes may have different identities/friendly names and Wi-Fi credentials configured from setup AP; no home password is compiled into the source.

**Pi companion:** house-audio-server branch feature/audio-input-gateway includes a Python-only source registry/HTTP WAV proxy, separate from production server. It must be deployed and hardware-tested separately.

**Protocol:** node posts /register every 5s to the Pi gateway and hosts finite CD audio track WAV at :8080/track/<number>.wav. The Pi exposes /audio/<sourceId>/<track>.wav without altering MPD. Source and listening-node presence are separate.

**Local verification:** eight Python tests passed, including HTTP WAV proxy, validation and expiry. Archive SHA256 c810cd274c5cda000110c99a7fc5a9fa2d230efc6ca63cff6fdd67e358114cd2, containing Arduino source, Pi gateway, tests and instructions.

**NOT verified:** Arduino sketch compile/link, actual READ CD sector extraction, WAV sound, Wi-Fi throughput, MPD playback, source selection UI, analog ADC input, real Pi installation. Not a production release. Do not merge/deploy over working installations without field validation.
