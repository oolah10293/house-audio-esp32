# LiteOn CD player 3.1 — first over-the-air update

## Current hardware baseline
On 2026-11-13 the user confirmed the existing S3 was running **3.0**:
- source SHA: `7f2073f8c1df3167293f7482baadffc1e05e4031`
- sketch MD5: `b3127e9884d49a30f171ae066e2bd93c`
- free sketch space: `1310720` bytes
- Pi curl successfully retrieved the same identity from `http://192.168.1.175/api/firmware`.

This establishes identity/HTTP connectivity, **not yet OTA-write success**.

## OTA release 3.1
This release **only advances the embedded firmware identity** and thereby exercises the installed OTA update path. Audio playback, USB-SCSI drive access, and HTTP streaming logic are not changed.

The build is compiled by GitHub Actions on the exact commit in the build manifest, using esp32 Arduino core 2.0.14 and XIAO_ESP32S3. It includes `HouseAudioCDInput.ino.bin`, `SOURCE_MANIFEST.json`, and `update_cd_from_pi.py`.

## Installation on the Pi (no Arduino IDE, no USB)
Copy the single `LiteOn CD player 3.1.zip` from the computer where it was downloaded to the Pi, for example with Windows PowerShell:

```powershell
scp "LiteOn CD player 3.1.zip" ads-suck@192.168.1.12:~/
```

Run the commands from the folder containing the ZIP on Windows (otherwise supply its full Downloads path). This step only transfers a ZIP to the Pi; it does not install firmware.

On the Pi:

```sh
mkdir -p ~/liteon-ota-3.1
cd ~/liteon-ota-3.1
unzip -o ~/"LiteOn CD player 3.1.zip" HouseAudioCDInput.ino.bin SOURCE_MANIFEST.json update_cd_from_pi.py
python3 update_cd_from_pi.py HouseAudioCDInput.ino.bin --manifest SOURCE_MANIFEST.json --host 192.168.1.175
```

The updater checks the binary against its SHA-256 manifest, confirms old firmware identity, sends it to the ESP32 OTA receiver, and polls `/api/firmware` to verify the **new expected exact GitHub commit** after reboot. A successful HTTP POST alone is not treated as proof.

**Only run OTA from the Pi at 192.168.1.12.** The ESP32 restricts firmware POSTs to that Pi IP. Do not paste passwords or send the ESP32 binary from the PC.

## Outcome
- If the script prints `PASS: ESP32 reports expected running commit`, the firmware identity verifies that the update booted.
- If it prints an error or timeout, **stop and share the complete output; do not repeat the update blindly**.
- No physical flash/OTA acceptance is claimed until the user runs this and confirms success.
- Existing Wi-Fi credentials remain in ESP32 nonvolatile storage, not in the generic binary.

## Version label and contents
Do not merge this package with an old Arduino sketch folder. Do not touch `LocalNetworkConfig.h`; no Arduino installation is needed. The ZIP includes source and documentation for recovery, but the normal OTA path uses only the three files listed above.
