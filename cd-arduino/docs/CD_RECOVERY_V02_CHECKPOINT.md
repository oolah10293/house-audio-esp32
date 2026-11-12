# CD source input v0.2: readiness diagnostics and automatic recovery

**2026-11-12, development checkpoint.** The user explicitly authorized rebuilding the XIAO ESP32-S3 CD source firmware after field testing showed: direct home Wi-Fi and Pi registration HTTP 200; a disc's 11 tracks detected; Track 1 (about 28 MB) downloaded through the Pi and played normally on a PC. In subsequent testing, tracks appeared and disappeared and a manual restart of the XIAO sometimes restored them. The exact root cause of failures is **not yet verified**.

## Changes (Arduino IDE sketch only)
- Retain the previous verified direct home-Wi-Fi configuration and Pi gateway protocol on port 8768.
- Preserve USB MSC/SCSI driver files, 44.1 kHz/16-bit stereo WAV serving, and READ CD data path **byte-for-byte**.
- Display TEST UNIT READY transport error code and REQUEST SENSE key/ASC/ASCQ on node web status page; show count of consecutive not-ready checks and automatic drive-start attempts.
- Retain cached tracks through four 2-second consecutive failed ready checks (up to about 8 seconds). On a prolonged failure mark the source unavailable, instead of immediately erasing the track list after the first error.
- After three failed checks, issue a non-ejecting SCSI START STOP UNIT Start=1 and retry readiness; throttle to at most one attempt per 30 seconds. Do not issue the start command when successful sense reports MEDIUM NOT PRESENT (sense key 0x02, ASC 0x3A).
- Re-read the CD table of contents when ready status returns, and continue automatic periodic checks. No host-library teardown/reset attempted because it could interfere with an active WAV read.
- Rescan button remains available; no setup access-point network. Real credentials stay local in LocalNetworkConfig.h and out of version control.

## Boundaries and test status

This is a diagnostic/recovery experiment, **not proof the CD drive has been fixed**. Only hardware tests can establish whether drive-start recovery works for the Lite-On, or whether the low-level USB transport requires further repair. The updated code does not attempt USB host re-enumeration after the host itself fails to open the drive.

GitHub Actions build: `arduino-cd-input-check.yml` with `esp32:esp32@2.0.14` / `esp32:esp32:XIAO_ESP32S3`. The first attempt failed a warnings-as-errors format truncation diagnostic; it was corrected by explicitly bounding diagnostic string formatting. **PASS**: final CI run [43162362226](https://github.com/oolah10293/house-audio-esp32/actions/runs/43162362226) built the exact delivered sketch with user-pinned Arduino ESP32 core 2.0.14. Git blob SHA of sketch: `7f29c98c897e3c9f865e7b6fa92406e4e250cd7d`. No physical XIAO flash or CD recovery field validation has been performed for v0.2.

## User delivery
Standalone Arduino sketch ZIP `HouseMusic_CD_Input_AutoRecovery_v0_2_XIAO.zip`. Includes a single placeholder `LocalNetworkConfig.h` and all six USB/SCSI driver files; excludes the redundant example config header. User should copy their already-working credentials from their previous `LocalNetworkConfig.h`. No Pi changes, production main merge, or hardware flashing has been performed by the assistant.
