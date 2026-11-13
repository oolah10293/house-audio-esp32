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

## Authorized 3.1 build checkpoint — 2026-11-13
- Explicit user authorization for this one subsequent OTA verification image was given in conversation.
- Exact compiled source commit: `01452e660f3e8fec5d30ba84823cc0cfb5d2d58e`.
- GitHub Actions run: https://github.com/oolah10293/house-audio-esp32/actions/runs/43476480059 — **PASS**.
- Arduino core 2.0.14 / XIAO ESP32-S3 compile and checked ZIP integrity **PASS**.
- Compiled OTA application image: **798,336 bytes**; no private SSID/password embedded.
- Release archive: `LiteOn CD player 3.1.zip`, **544,921 bytes**.
- ZIP SHA-256: `bcae13b3ce1a37cfa5c5bc3beadab7e555f4649ea6ecdbe081329f6cef7597ed`.
- Hardware OTA installation **not yet verified**; do not declare success until the Pi updater reports exact new commit after reboot.
- Stream-replacement code unchanged from user-reported 3.0 build.

## One-command Pi installer delivery checkpoint
- Authorized by user in conversation after failed cross-machine transfer attempts; use **one command on Pi only**.
- Pi installer source: `cd-arduino/tools/install-liteon.sh` (commit `7afd68ee86023803bad6f8c2fc0716d8190c1ae1`).
- GitHub release tag: `liteon-cd-3.1`, precompiled package asset named `LiteOn.CD.player.3.1.zip`. Public repository release resource confirmed by GitHub API.
- Publisher workflow run: https://github.com/oolah10293/house-audio-esp32/actions/runs/43479446225 — **PASS**. Its workflow SHA was `0b125b18629a237c62b083ab2db7400df91e2cea`.
- Publisher verified release ZIP SHA-256 `bcae13b3ce1a37cfa5c5bc3beadab7e555f4649ea6ecdbe081329f6cef7597ed` and embedded source SHA `01452e660f3e8fec5d30ba84823cc0cfb5d2d58e`; **no firmware recompile**.
- Installation command on Pi:
  `curl -fsSL https://raw.githubusercontent.com/oolah10293/house-audio-esp32/fix/liteon-usb-transport-recovery/cd-arduino/tools/install-liteon.sh | bash`
- Installer downloads asset into a temporary directory, verifies pinned package SHA, extracts binary/manifest/updater, uses existing Pi OTA uploader with post-reboot exact-commit confirmation, and cleans up. It does not use Windows or require manual ZIP transfer.
- **Pi execution not yet verified on actual machine**. GitHub API verifies publication; no hardware update assumed until Pi prints `PASS`.
