# Bose automatic standby v0.1.0

This is the Bose firmware step after house-audio-server v0.12.0. It does not
change regular S3 nodes, the Pi queue, or the Android app. Physical acceptance
remains pending until this build is installed and tried on the Bose.

## Installation

The sub is already registered on the Pi. Replace the Bose device YAML with
`firmware/bose-subwoofer-prototype.yaml`, retaining the existing `wifi_ssid` and
`wifi_password` secrets, and install wirelessly using ESPHome. Its device name
remains `bose-sub`. No new ID, key, or Home Assistant dependency is required.
ESPHome 2026.4.0 is the build baseline and minimum version.

The component discovers its existing sub registration by the exact uppercase,
colon-separated station MAC used as hello.id by the pinned Snapclient core.
It does not guess based on a friendly name or IP. The default Pi origin is
`http://192.168.1.12:8787`, configurable with `house_server_url`.

Open **http://192.168.1.12:8787/subwoofers** for Pi demand and reported state.
Home Assistant now has **Bose Mode** (Auto / Force standby / Force awake) and
**Bose Amplifier Status**. The old **Bose Standby (Test)** entity is replaced.
The five tuning controls retain their IDs, ranges, defaults and restore behavior.
Mode also restores; a new installation defaults to Auto. Forced mode persists
until Auto is selected again, including over a reboot.

## Audio source decision

Keep the pinned PR #14389 Snapclient source in the YAML. The requested candidate
`c-MM/esphome-snapclient@main` was inspected: it uses top-level `snapclient` and
CarlosDerSeher core `1adc5245`, while this crossover wraps the different
PR #14389 two-argument PCM-fragment/settings DSP ABI in luar123 core `77426800`.
The existing adapter explicitly rejects the c-MM component. Merely changing the
source would break this Bose configuration. No audio-stack migration is included.
Mono crossover, software volume, phase and bypass processing are unchanged.

## Behavior

- Auto wakes for fresh authoritative demand from any assigned normal speaker.
- Confirmed absence starts one 30-second timer. Continued absence does not
  restart it. Presence cancels it; missing/invalid/stale demand cancels it too.
- After an outage, a new confirmed absence starts a full new 30 seconds.
- Unknown demand holds the current request/state. At boot, without fresh demand
  or Force awake, the component requests standby after the 3-second startup wait.
- Force standby/awake override demand. Networking and Snapclient remain powered
  and connected. The Pi remains the sole session authority.
- Pi polling is every 2 seconds; freshness is at most 10 seconds. Feedback is sent
  on transitions, every second during a countdown, and at least every 4 seconds
  while steady, subject to connectivity. Pi reports expire at 15 seconds.
- HTTP runs in a low-priority FreeRTOS task with bounded response size/timeouts.
  Fixed-size queues exchange snapshots with the ESPHome main loop. Old snapshots
  are not replayed to make stale feedback look fresh.

## Command timing and evidence

All wake sources use the same command engine. The proven wake/wake/analog/stream/
attenuation byte sequences, 50 ms initial idle, 180 ms between wake commands
(30 + 150), and 30 ms after other frames are preserved. Waiting yields to ESPHome.
Only one 4- or 5-byte frame is bit-banged per loop operation, about 8.5–10.7 ms.
Mode/slider callbacks only update requests; they do not transmit.

Known bus traces in repository issue #4 show exact transmitted bytes echoed,
then a trailing `00` acknowledgment. This implementation requires the entire
frame plus that separate trailing byte; a zero within the echo cannot qualify.
It discards stale RX before sending and accepts a relevant reply inside a 120 ms
window. Missing/rejected/malformed replies retry the whole wake sequence after
1 second, up to three attempts. Unchanged failed state does not spam commands;
reselecting a mode explicitly retries. Requests cannot overlap on the bus.

**Acknowledged means command acceptance, not measured physical amplifier power
or proof of audible sound.** A partial wake never marks the full wake successful.
Attenuation writes use the same serialized bus engine.

## Verification and field checks

Native tests cover boot, wake order/minimum gaps, grace/cancellation, unknown and
expired demand, manual precedence, retries, rapid mode reversal, echo-only/lone-zero
rejection, attenuation and millisecond rollover. JSON tests reject wrong identity,
unknown/malformed demand and unsafe IDs. Existing DSP/ABI checks remain. CI builds
the exact pinned YAML and verifies DSP wrapper call sites in the resulting ELF.

On the hardware, verify:

1. Existing registered card receives feedback; HA and Pi state agree.
2. Force standby then Force awake restores audible bass on the **first request**,
   including repeated cycles and quick changes. Check the final state and logs.
3. Auto follows an assigned speaker. Last assigned speaker leaving starts the
   countdown; return during the countdown cancels it.
4. Outage holds state; cold boot without demand does not unconditionally wake.
5. Tuning settings restore and bass stays synchronized with a normal speaker.
6. The old 404 ms API warning is gone. Record this separately from audible-wake
   success. Tests and a successful compile do not prove physical acceptance.

If replies differ from the known echo-plus-00 framing, collect logs before
weakening acknowledgment validation. The timeout is bounded; this is not a claim
to a fully documented Bose protocol specification.
