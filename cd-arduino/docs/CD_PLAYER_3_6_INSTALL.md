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
