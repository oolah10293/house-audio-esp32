# CD input: physical event identity for House Music plug-and-play (2026-11-12)

**Source-only checkpoint.** Based on the user-verified v0.2 Lite-On/XIAO
driver-recovery sketch in `feature/reusable-input-node`. No new firmware
was compiled, packaged, flashed, or physically tested. The earlier v0.2
firmware remains on the hardware.

## Why this exists

The previous gateway learned only an ESP device ID, online/offline state,
and a list of audio track numbers. A second disc with the same number of
tracks looked identical. The server also could not tell a new power-on
from a five-second heartbeat, risking involuntary track-one restarts or
manual source overrides.

## New registration fields

- `bootSession`: 16 lowercase hexadecimal characters from two random
  32-bit values generated at each XIAO power-on.
- `discRevision`: incremented on the first readable medium of a boot
  session; on newly detected media after SCSI MEDIUM NOT PRESENT; on drive
  disconnect/reconnect; and when a later READ TOC fingerprint changes.
  Ordinary heartbeats and a same-disc Rescan do not increment it.
- Changes to existing source ID, IP address, track list, WAV URL, audio
  format (44.1kHz, 16-bit, stereo), and CD sector reader are not intended.

The TOC fingerprint uses 32-bit FNV-1a over the returned table data. It
is not a cryptographic identifier. While no stream is being extracted,
the firmware periodically rechecks TOC at 15-second intervals to catch
rapid swaps missed by its two-second readiness polling. While WAV is
being streamed, this periodic check is suppressed to avoid fighting
the audio read operation.

## Server/gateway compatibility

This version requires the paired source-only gateway update from
`house-audio-server` branch `feature/cd-auto-takeover`. The gateway
relays both fields without trying to control MPD. That branch's Pi
controller decides whether to take over the whole-house stream or yield
to a deliberate manual source selection.

If the XIAO boots without a CD, it registers status `no_disc` and
`discRevision=0`. Once a readable disc appears, revision becomes
positive. A new boot session resets revision naturally.

**Known limitations:** Two different discs with identical TOC values and
a completely missed eject/reinsert interval cannot be distinguished.
Unplugging the drive and instantly reconnecting before device
disconnect detection may also leave the same generation if the SCSI
status and TOC are indistinguishable. Hardware acceptance tests remain
necessary; the 15-second check is not proof of reliable hot swapping.

No Arduino workflow was triggered for this source-only development
branch, and the user has not authorized compiling, flashing, or
packaging this new version.
