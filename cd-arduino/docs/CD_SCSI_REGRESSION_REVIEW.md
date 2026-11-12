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

## Deeper transport-path audit (2026-11-12)

Observed `0x2` is compatible with the USB host transfer-status enum `USB_TRANSFER_STATUS_TIMED_OUT` (enum order: COMPLETED=0, ERROR=1, TIMED_OUT=2), not decoded SCSI REQUEST SENSE key 0x02. This is inferred from source returning raw transfer status and the documented enum order, not confirmed by a serial transfer trace.

`usbhost_scsi_testUnitReady` and `usbhost_scsi_startStopUnit` both call `usbhost_cmd_cbwExecute` with zero data length. That uses `usbhost_bulkTransfer` for the 31-byte command block (CBW) and then the command-status wrapper (CSW). In the existing code, a CBW error is returned immediately, while a CSW-stage timeout can be lost behind an invalid-CSW `ESP_FAIL`. Thus the shared `0x2` for these zero-data commands **most strongly suggests a CBW bulk-transfer timeout**. REQUEST SENSE also uses the CBW path, although its data/CSW phases differ. Precise stage requires the existing serial `usbhost_bulkTransfer` log or additional diagnostics to confirm.

The driver starts a 200-ms timeout for CBW writes and always uses 200 ms for the CSW following data-stage transfers; these timeouts are *unchanged* from previous v0.2. The UI's message `ERROR (0x2)` does not distinguish host scheduling delay from drive response delay or USB library timeout. Driver `usbhost_waitForTransDone` stops and flushes the endpoint when its semaphore wait times out. This does not prove physical cable/power fault.

The added periodic TOC polling is gated behind a successful readiness response, so with `discRevision=0` it has not successfully found a disc in that boot. The additional code generates a random boot nonce and starts event metadata, but initialization order and the USB task priorities are unchanged. No demonstrated root cause yet. For a focused next experiment, gather serial output showing the first USB CBW timeout and preceding enumeration, or add source-only transfer-stage diagnostics before any firmware build. Preserve the working Pi. 
