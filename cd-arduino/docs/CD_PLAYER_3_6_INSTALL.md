# LiteOn CD player 3.6 — device identity correction

**Authorization:** On 2026-11-23 the user explicitly authorized only
the Lite-On S3 advertised-name correction and a fresh build and OTA delivery.
The desired exact display name is **LiteOn CD Player**.

## Minimal change

- Change `SOURCE_DISPLAY_NAME` in
  `cd-arduino/HouseAudioCDInput/LocalNetworkConfig.example.h`
  from `CD Player` to `LiteOn CD Player`.
- Change `CD_RELEASE_VERSION` in
  `cd-arduino/HouseAudioCDInput/src/BuildIdentity.h` from `3.5` to `3.6`.
  The authorized CI build generates `BuildIdentity.h` with the compiled SHA.
- The existing Arduino sketch already assigns `friendlyName =
  SOURCE_DISPLAY_NAME` and sends that value as `name` in the existing Pi
  `/register` request. Therefore no server aliases, web UI special cases,
  source-id changes or new registration APIs are needed.
- Update the marker-gated Arduino CI build/package/publish workflow to 3.6
  and assert the intended source name from the compiled template.
- No Wi-Fi settings, USB/SCSI behavior, audio streaming, disc detection,
  automatic takeover, gateway or House Music server changes.

## Build and OTA acceptance gate

Use the existing `arduino-cd-input-check.yml` marker-only trigger:
`BUILD_AND_PACKAGE 20261123-liteon-3-6`, after the source and CI
workflow merge to main. This must compile exact marker commit SHA using
`esp32:esp32@2.0.14` for `XIAO_ESP32S3`, validate all archived
source/binary hashes, and publish `LiteOn.CD.player.3.6.zip` to tag
`liteon-cd-3.6` only after checks pass. Do not confuse compile success
with field verification.

OTA follows the existing Pi-only upload route at
`http://192.168.1.175/update`, using the existing checksum-verified
`cd-arduino/tools/update_cd_from_pi.py` helper; the S3 preserves its
home Wi-Fi settings in NVS. This image is **not** a USB flash.

Physical OTA and acceptance checks performed by the user:
1. Stop CD playback before updating.
2. Confirm before/after firmware SHA and version from `/api/firmware`.
3. Verify `http://192.168.1.12:8787/inputs` reports the same `sourceId`
   with `name: LiteOn CD Player`, after S3 re-registration; refresh Sources.
4. Test playable CD track, auto-takeover and return behavior unchanged.

3.5 remains the functional rollback baseline. No physical OTA success
is claimed without actual Pi output. Full release SHA, CI run, hash and
user acceptance are to be recorded after they exist.

## Verified build and release — 2026-11-23 (US Eastern)

- Compiled **exact source commit**: `6aeecb5314b51595f2db2d3b444ce9fc591d1af1`,
  the marker-only authorized commit following merged source PR #9.
- [GitHub Actions run 45310557918](https://github.com/oolah10293/house-audio-esp32/actions/runs/45310557918):
  **PASS** for marker authorization, pinned Arduino core installation,
  XIAO ESP32-S3 compile, source/package/binary integrity checks, artifact
  upload, and release publication.
- Published package:
  [LiteOn.CD.player.3.6.zip](https://github.com/oolah10293/house-audio-esp32/releases/download/liteon-cd-3.6/LiteOn.CD.player.3.6.zip).
  GitHub published asset size **548,021 bytes** and SHA-256
  `798d4ff5921a93a493e7ea1cc76ca3fc6085f0298c4f4ea4d3b6848e39a5ffe9`.
- Existing `cd-arduino/tools/install-liteon.sh` has been updated to pin the
  **exact URL and SHA-256**. The Pi installs via the existing
  `update_cd_from_pi.py` helper, which validates the binary digest and
  requires that the S3 report the expected compiled source SHA after reboot.
- **Hardware OTA and post-flash source-name registration are still
  unverified**. The user must run the installer on the House Music Pi.
  Do not claim installation based on the GitHub release alone.

### OTA installer

Stop CD playback. On the Pi run:

```sh
curl -fsSL https://raw.githubusercontent.com/oolah10293/house-audio-esp32/main/cd-arduino/tools/install-liteon.sh | bash
```

The script downloads and verifies the pinned ZIP then runs Pi-origin OTA
against `192.168.1.175` by default. It does not USB-flash or require
credentials to be disclosed. The device persists its home Wi-Fi in NVS.

Success requires `PASS: ESP32 reports expected running commit:
6aeecb5314b51595f2db2d3b444ce9fc591d1af1`, followed by a gateway
registration where `name` is **LiteOn CD Player**. Confirm the same node is
still available on the Sources page and audio/automatic takeover work.

## Pi OTA identity result — user report, 2026-11-23 (US Eastern)

The user supplied the following output from the pinned Pi-to-S3 OTA helper:

```text
Image accepted; waiting for reboot and firmware identity...
PASS: ESP32 reports expected running commit: 6aeecb5314b51595f2db2d3b444ce9fc591d1af1
Running image MD5: 52692fc99f66637743dfba3f982b3c6c
```

**PASS — running application identity following OTA**, based on the user's
actual Pi terminal output. This verifies that the S3 rebooted and returned
the expected source identity; the updater reports MD5
`52692fc99f66637743dfba3f982b3c6c`.

**Still unverified:** The gateway's current registered `name` field, the
Sources page rendering **LiteOn CD Player**, and post-flash CD track playback,
automatic CD takeover and session restoration. These require new field
observations, not assumptions from the compile or source identity.

## Name display accepted — owner report, 2026-11-23 (US Eastern)

After the verified 3.6 Pi OTA, the user confirmed: **"Nice work. The name
is correct now."** The intended **LiteOn CD Player** name is therefore
**PASS by owner observation on the House Music Sources page**, consistent
with the S3 registering `SOURCE_DISPLAY_NAME` and the gateway/webpage
displaying that name. No raw `/inputs` payload was supplied, so the
gateway's individual JSON fields have not been independently inspected.

**Still open:** Post-3.6 physical CD track playback, automatic takeover and
session restoration have not been reported in this acceptance update.
