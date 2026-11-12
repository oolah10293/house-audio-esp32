# Lite-On CD recovery package (2026-11-12)

This branch restores the exact Arduino sketch and USB/SCSI transport from the user-field-proven v0.2 recovery checkpoint, before event-identity code was introduced. It is a rollback to the last known functional version, not a claim that the intermittent USB failures have been conclusively resolved.

- Working sketch SHA: `7f29c98c897e3c9f865e7b6fa92406e4e250cd7d`.
- Newer event-identity sketch showing failures: `161f5af5d27dd7169d245892f887c3edd72df18a`.
- Arduino core: `esp32:esp32@2.0.14`, board `XIAO_ESP32S3`.
- Use existing `--no-stub` upload workaround.
- The ZIP artifact contains complete `HouseAudioCDInput` sketch including six USB/SCSI source files, the audio transport header, and a placeholder `LocalNetworkConfig.h`. Restore your previously working local Wi-Fi credentials before uploading.
- The new Pi auto-takeover feature requires bootSession/discRevision and will NOT auto-play from this old firmware; manual CD discovery and track audio path remain the recovery scope.
- Do not modify the currently running Pi during the node recovery.
- Compilation is checked on CI; physical USB enumeration, TOC reading, and WAV playback require a new user hardware test.

The branch's workflow packages `HouseMusic_XIAO_CD_Recovery.zip` after Arduino compilation. No newer auto-media-event sketch is included.
