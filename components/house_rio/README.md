# Rio subwoofer: Pi-demand software audio following (source draft)

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

## Important: one-time Pi registration after the native-MAC change

The existing Rio registration on the Pi refers to the OLD spoofed
`44:BD:8D:EC:51:80` ID. Without the override, Snapserver will recognize
the replacement XIAO S3 by its real Wi-Fi MAC. The current Pi API refuses
to change an existing subwoofer's `rendererId` and offers no delete route.

At an explicitly authorized deployment, allow the Rio's real renderer ID
to appear and register it as a new dependent Rio subwoofer in the Pi web
interface with the original Motorola Radio parent and intended fallback
volume. This leaves the old registration stale; a later Pi cleanup is
required to remove or migrate it cleanly. Never claim the existing
assignment will follow the name or automatically retarget.

## Testing/deployment status

This is source work only. The replacement ESP32 has **not** been reflashed,
and the Pi has **not** been updated by this change. No compile, flash or
runtime validation was performed. Removing MAC spoofing and pinning
dependencies do NOT by themselves prove a fix for the 2.4-GHz interference
or airtime-utilization problem. Do not deploy the new
status values unless the Pi's companion server change is installed first.

Before treating it as accepted firmware, validate ESPHome compilation against
the pinned Snapclient branch, then bench-test no-parent mute, parent arrival,
parent departure, server outage hold, timing and tuning. No firmware image,
installer, release or deployment was produced here.
