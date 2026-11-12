# LiteOn CD stream / USB recovery source checkpoint — 2026-11-12

## Corrected field observation
The user corrected an earlier report: after an ESP32-S3 power cycle, **MP3 playback resumed from the Pi**, but the CD player **still reported no disc**. A manual CD rescan also failed to recover tracks. Do not claim that an S3 reset restored CD playback or demonstrated USB-host-only recovery. The underlying drive/media/host failure remains unisolated.

Other observations: complete direct WAV 43,229,804 bytes in 82.55s with House Music stopped; periodic mid-track silence with MPD time stalling/House Music clock looping; gateway 502s; source registered while tracks=[] and status=no_disc; ESP reported TOC success with zero recognized audio tracks and five drive-start attempts.

## Source commits (branch `fix/liteon-usb-transport-recovery`)
- `18e1128386b3136fb85edabc7c4b2ad49f5d1fbe`: reusable audio HTTP transport acquires optional source lease for the full GET WAV stream, releases after streaming. HEAD does not reserve SCSI device.
- `10f4c0a5525ca9745792d79ea7ea032dd329dcb1`: CD sketch holds `cdIoMutex` for entire polling cycle; audio holds the same mutex over the entire stream. Ambiguous zero-track TOC result no longer clears prior good track list; explicit rescan requests TOC without deleting tracks; transport failures no longer discard tracks without decoded medium-not-present sense. Adds READ CD error details.
- `3f22c8d18e5bc8fd7b8a5fc7f0d70f62394d3f5f`: USB Bulk-Only Mass Storage recovery now reports and propagates failed endpoint-clear operations.

Existing individual-command `scsiExeLock` remains in place; new stream lease addresses **interleaving**, not simultaneous SCSI command execution. Existing CBW/CSW provisional 1500ms timeout remains unchanged.

## Review / verification
- Reviewed source snippets from the committed branch: both lock acquisition and release paths; poll loop releases before its delay; zero-track path returns without writing the previous track list; no new automatic USB reset or restart was added.
- **No Arduino compilation, static toolchain checks, hardware tests, firmware package, ZIP, flash, or Pi deployment** was performed or authorized. These changes are source-only and unverified on hardware.
- Not yet known whether preventing command interleaving fixes CD stalls; the physical drive may independently lose usable media state. Ensure the drive has a readable disc before attributing any future result to code.

## Future acceptance when user authorizes build
Compile exact committed revision; inspect diagnostics; test CD detection, complete streamed playback over multiple tracks, normal next/previous, eject/reinsert and changed disc, manual rescan, non-disc MP3 restoration, and deliberate transport failures. Monitor USB serial READ CD details and failures of BOT reset/endpoint clear. Avoid stronger auto-reset/re-enumeration until evidence supports it.


## Next revision plan (planning only; no implementation or build authorization)
Add both **bounded READ CD retries** and **read-speed control** to the LiteOn CD firmware:
- Retry failed or short CD audio reads a limited number of times, record the failing LBA and transport/SCSI error, and avoid indefinite hangs. When retries are exhausted, report the failure and let another track be selected without erasing the disc list solely because of a read failure.
- Request a moderate audio-CD read speed using the existing `usbhost_scsi_setCDSpeed()` command (initial target 4× for hardware evaluation). Verify whether the LiteOn honors it and whether streaming remains fast enough. Speed is an operating parameter, not an assumed cure for read failures.
- Coordinate read retries, drive commands, and speed setting under the existing stream/polling synchronization. Preserve prior CD autoplay, next/previous, and Pi library restoration behavior.
- Assess failure and recovery on the drive with hardware evidence; an ESP32 restart did **not** recover CD detection in the reported case.

These are planned changes, not yet coded, compiled, packaged, or tested. Explicit authorization is required before a firmware build.
