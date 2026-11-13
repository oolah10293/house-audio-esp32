# LiteOn CD player 3.0 — first installation and future OTA updates

## Verified and not verified
The release archive is generated from the exact authorized GitHub Actions commit. Arduino core 2.0.14 / XIAO_ESP32S3 compilation and archive checks are enforced by CI. Physical USB installation, CD playback, and the first real OTA update are NOT YET hardware verified.

## First installation — one USB upload
The S3 currently running older firmware has no OTA endpoint. The new version must be installed once by USB.

1. Extract `LiteOn CD player 3.0.zip` on the Windows PC.
2. Open the existing Arduino IDE CD sketch folder using **Sketch > Show Sketch Folder**. Your working, private `LocalNetworkConfig.h` is in this folder. **Do not replace it or retype the Wi-Fi password.**
3. Copy the files from the new archive's `HouseAudioCDInput` folder into this existing sketch folder, **except `LocalNetworkConfig.h`**, allowing updates to the .ino, src headers, and `partitions.csv`. This keeps the same sketch path, USB driver and Wi-Fi configuration.
4. Restart/open `HouseAudioCDInput.ino` in Arduino IDE. Keep the previously working XIAO ESP32-S3, Arduino core 2.0.14, port, and existing `--no-stub` upload workaround.
5. Upload once via USB. The new `partitions.csv` in the sketch forces dual OTA application partitions, independent of the prior large-app selection. The existing SSID/password are saved in ESP32 NVS on boot for future updates. (The exact behavior on this hardware remains to be tested.)
6. With USB disconnected and CD drive connected normally, visit `http://192.168.1.175/api/firmware` (or the device's actual DHCP address). Confirm `version`, full `sourceSha`, and `sketchMd5`.

**Do not install the standalone compiled .bin onto the currently flashed single-app partition layout.** It will not establish a dual-slot OTA layout. Use the Arduino sketch upload for the one-time migration.

## Subsequent OTA updates — from the Raspberry Pi, no Arduino IDE or USB
The archive includes `HouseAudioCDInput.ino.bin`, `SOURCE_MANIFEST.json`, and `update_cd_from_pi.py`.

Transfer these three files to a directory on the House Music Raspberry Pi and run:

```sh
python3 update_cd_from_pi.py HouseAudioCDInput.ino.bin --manifest SOURCE_MANIFEST.json --host 192.168.1.175
```

The script checks the image SHA-256 against the manifest, queries the running version, sends the firmware to `/update` from the Pi, and then checks that the ESP32 reports the *new* expected source commit after reboot. A successful HTTP upload alone is not treated as proof of installation.

The ESP32 accepts OTA writes only from the configured Pi IP (`192.168.1.12`). This is local IP restriction, not cryptographic authentication. Only use this facility on a trusted home LAN.

The generic CI-compiled .bin contains **no private Wi-Fi password**. The ESP32 uses network credentials saved in NVS by the first USB installation.

## Recovery
If a generic binary is installed without saved NVS credentials, the S3 starts the `LiteOn-CD-Setup` temporary access point, password `LiteOnCDsetup`. Connect and open `http://192.168.4.1/setup` to save home network settings one time.

If an update does not reboot into the expected commit, stop and report the console output. Do not repeat the upload blindly. USB reflash remains the fallback recovery path.

## Scope
This release adds identity, network provisioning, and OTA infrastructure. It intentionally does **not** change the existing CD track streaming or stream-replacement logic.
## Build/release checkpoint — 2026-11-13

- **User authorization:** explicit in the current conversation, covering implementation, compilation, packaging, and delivery of firmware identity plus Pi-based OTA infrastructure.
- **Compiled source SHA:** `7f2073f8c1df3167293f7482baadffc1e05e4031`.
- **GitHub Actions:** https://github.com/oolah10293/house-audio-esp32/actions/runs/43466783590 — **PASS** (Arduino 2.0.14 XIAO ESP32-S3 compile, exact-commit manifest and package verification).
- **Firmware application size:** 798,336 bytes; compiled against 1,310,720-byte OTA app-slot limit.
- **Packaged ZIP SHA-256:** `633073ca7e5a56c5ac2eb201533becd62d2c3565b3e29ff7fec8f775ec1b0bc4`.
- **Delivered files:** `LiteOn CD player 3.0.zip`, containing the compiled binary, versioned source sketch, fixed OTA partitions.csv, local Pi updater script, installation documentation, and checksummed manifest.
- **Physical tests:** **NOT YET PERFORMED**. No claim of successful initial flash, runtime Wi-Fi retention, OTA upload, or CD playback validation.
- **Streaming changes:** none; the existing track-switching problem remains a separate issue.
