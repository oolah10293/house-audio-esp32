# Lite-On readiness failure — source comparison checkpoint (2026-11-12)

## Hardware report (user screenshot)
After flashing the new event-identity firmware on the XIAO ESP32-S3, its web page at 192.168.1.175 showed:
- Wi-Fi connected; Pi gateway registration HTTP 200
- CD start command failed; retrying
- TEST UNIT READY ERROR (0x2); REQUEST SENSE ERROR (0x2); START STOP UNIT ERROR (0x2)
- 16 consecutive not-ready checks, 2 recovery attempts
- bootSession populated, discRevision 0, no CD tracks.
The user reports USB connection and drive power are not the issue. No corresponding serial trace has been supplied. This is a field report, not independent hardware reproduction.

## Direct source comparison
Compared `feature/reusable-input-node` (previous user-tested v0.2) against `feature/cd-auto-media-events`.
- `cd-arduino/HouseAudioCDInput/src/usb_host_msc/usbhost_driver.c`: same blob 0246b14db93aaec7e4fe671a31b892654f043e48.
- `cd-arduino/HouseAudioCDInput/src/usb_host_msc/usbhost_scsi_cmd.c`: same blob f8a13f0d4ed215484320d972274d143a0d396275.
- `cd-arduino/HouseAudioCDInput/src/usb_host_msc/usbhost_msc_cmd.c`: same blob 3756d19025ce3d5010c532bf83962ce9697ba999.
- Main sketch changed from 7f29c98c897e3c9f865e7b6fa92406e4e250cd7d to 161f5af5d27dd7169d245892f887c3edd72df18a. Additions relate to bootSession, discRevision, TOC fingerprint and conditional 15-second TOC rechecks. Existing `usbhost_driverInit()`, readiness/sense/start calls and drive-start retry path are unchanged.
- Shared audio transport header adds an in-progress flag checked before scheduled TOC rechecks. It does not add an initial USB setup action.
- The new TOC recheck is gated on successful TEST UNIT READY and cannot itself directly explain failures before the first readiness success.

## Assessment and boundary
No source-level proof of a newly introduced SCSI command regression. Previous v0.2 hardware checkpoint reported intermittent loss of disc detection, although it also showed a successful automatic recovery and track detection. The observed SCSI failure could reflect a persistent/occasional state in the USB host/SCSI transport; root cause is **not established** and should not be mislabeled as external power or cable fault without evidence.
No new code, firmware build, packaging, flash or Pi deployment was performed during this review. Next diagnostic should focus on actual SCSI transport return path for code 0x2 and ordering/concurrency of USB host readiness and audio operations, with an independent hardware acceptance check before any claim of repair. Do not touch the Pi until the node identifies and serves CD tracks.
