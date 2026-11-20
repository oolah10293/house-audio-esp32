# Rio subwoofer: Pi-demand software audio following

The existing Rio is a XIAO ESP32-S3 / PCM5102A / TPA3118 node. Its amplifier
has **no mute or standby control line wired to the ESP32**. Consequently this
implementation **does not power the amplifier off**. It mutes the processed
PCM stream, with a 10 ms fade on transitions, while leaving the Snapcast
connection, transport, and audio stream synchronized.

Source configuration: [firmware/rio-subwoofer.yaml](../../firmware/rio-subwoofer.yaml)
(uses the ESP32's native MAC; no MAC spoofing).

## Behavior

- At boot, start with DSP audio muted until the Pi returns valid demand.
- Discover Rio's registered subwoofer ID from `GET /api/subwoofers`
  by matching its native Snapcast renderer ID, just as Bose does.
- The ESPHome Snapclient source is pinned to the exact immutable revision
  already used by Bose. Rio's house-specific DSP and follow components are
  also pinned to an immutable revision (no moving branch).
- Rio's periodic Pi demand/status reports remain, as they are required for
  automatic follow; they do not control the TPA3118 power supply.
- Poll `GET /api/subwoofers/<id>/demand` every 2 seconds in a separate
  FreeRTOS task (not in the ESPHome main loop).
- When the assigned normal speaker (currently Motorola Radio) is present/
  eligible, fade the Rio sub audio in. When it is absent, mute immediately.
- Invalid, expired or unreachable Pi data means **unknown**. Hold the
  previous audio mute state, rather than assuming the speaker left.
- POST `auto` mode status to `/api/subwoofers/<id>/status` with
  `audio_active` or `audio_muted`; do not claim amplifier acknowledgments.
  Home Assistant also has a read-only `Rio Audio Follow Status` sensor.
- The sub remains a dependent Snapcast renderer and does not create its own
  listening session. DSP tuning, phase, and bypass controls remain intact.

The `start_muted: true` option is specific to Rio. It defaults to false
for existing Bose DSP configurations; this change does not touch the Bose
SmartSpeaker standby firmware.

## Hardware limitation

DSP mute is silence, **not 20 V standby**. The TPA3118 will still draw its
idle current. Hardware standby would require a deliberately wired
enable/mute circuit or another external power control, neither of which is
part of this change.

## Native-MAC cleanup and Pi registration — November 14, 2026

The old Rio configuration overrode the replacement S3's native identity with
`44:BD:8D:EC:51:80`. Source commit
[`cb5cd61`](https://github.com/oolah10293/house-audio-esp32/commit/cb5cd61f2ac610ab032d28ce1056e12ac735fa01)
removed the `esp_mac.h` include and `esp_base_mac_addr_set` call. It also
aligned Rio's Snapclient with the Bose configuration and pinned Rio's DSP and
Pi-follow components to immutable revisions. The code preserves the Rio
DSP controls, Snapcast renderer, and parent-speaker audio-follow behavior.
[Documentation commit `4dab850`](https://github.com/oolah10293/house-audio-esp32/commit/4dab850eb7770c6b4b694d49c5380d454c80937a)
recorded those source changes before field testing.

Changing the MAC means the old Pi subwoofer registration no longer matches the
renderer. At source-commit time the old registration was still present, and the
documentation warned that the API did not provide a delete route. **Later the
user reported successfully deleting that old registration and registering
Rio with its real MAC.** The exact deletion method, registration data, and
resulting MAC address were not independently captured here; don't infer them
from the old spoofed ID or claim an automatic migration.

## Field validation — reported November 14, 2026

**Reported by the user after the source commits:**

1. The cleaned Rio configuration was installed onto the replacement S3.
2. The outdated Pi registration was removed and Rio registered using its
   native MAC identity.
3. CD-player S3 playback ran through six listening nodes, including Rio,
   for two songs **without any reported dropouts**.

These are positive **field observations**, not independently verified firmware
binary hashes, complete device logs, or a long-duration soak test. In
particular, the observation supports improved multi-node playback under that
test, **not proof** that the fake MAC or one particular YAML change caused the
earlier congestion. The 2.4 GHz issue appeared before cleanup, with removal
of the Rio node restoring playback; the underlying mechanism remains unresolved.

The source commits were explicitly **source-only** and did not themselves
authorize or perform compilation, flashing, installation, release packaging,
or Pi deployment. The subsequent user-reported device installation and
registration happened afterward. This separates the earlier "not flashed"
status from the later field outcome instead of leaving the dated source-only
status as if it were current.

## Still unverified

- Long-duration multi-node stability and AP channel-utilization behavior.
- Which specific change (if any) addressed the observed Wi-Fi dropouts.
- Compile logs/binary provenance for the subsequently installed Rio image.
- Full automatic audio-follow edge cases: parent leave/return, server outage,
  and restored DSP state after reboot.

For the audit trail, see
[November 14 Rio field checkpoint](../../docs/RIO_2026-11-14_FIELD_CHECKPOINT.md).
No firmware build, release, flash, or deployment was carried out as part of
this documentation update.
