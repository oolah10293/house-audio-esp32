# House Audio ESP32

ESP32-S3 synchronized audio renderer firmware for the whole-house music system.

These nodes are intended to hide inside vintage radios, stereos, powered speakers, or small standalone boxes and make them outputs for the single house playback session.

## Core production behavior

In the finished system, an ESP32 node is a **renderer**, not an independent music player.

On power-up it should:

1. boot
2. join the home Wi-Fi
3. discover the house-audio / Snapcast service on the local LAN
4. connect to the synchronized audio stream
5. fill its timing/audio buffer
6. begin output at the **current house playback timestamp**

If music is already halfway through a song when a node turns on, the node joins at that point. It does not restart the song and it does not create its own queue.

If the server is unavailable, the node should simply keep retrying. No phone interaction should be required.

## Permanent server relationship

The permanent house-audio server is the existing Raspberry Pi that already owns the music files locally.

Server-side music root:

```text
/mnt/sharedrive/John/Shared Music
```

That path is **server-side only**. The ESP32 does not browse or mount it.

The production path is:

```text
Raspberry Pi local files -> MPD -> Snapserver -> ESP32-S3 Snapcast client -> I2S DAC -> amplifier/stereo
```

The first ESP32 proof used the same permanent Snapserver instance intended to remain in production. No disposable proof server was used.

## Hard power-off is intentional

Many nodes will live inside existing radios/stereos whose original power switch physically removes power. That behavior is a design requirement, not a fault condition.

The ESP32 firmware must tolerate abrupt power removal with no shutdown sequence. Turning a radio off should make that output disappear immediately while the central house session continues elsewhere.

## Proven audio path

Production baseline:

```text
Wi-Fi -> ESP32-S3 -> Snapcast client -> I2S -> PCM5102A -> existing amplifier/stereo -> speaker
```

PCM5102A line-level output is now proven on two physical nodes. Existing amplifiers and analog volume controls are preserved where practical.

For mono equipment, stereo DAC outputs must be summed through resistors rather than tied directly together.

## SMB rule

The ESP32 node should **not** mount the music SMB share or build playlists in the planned production architecture. The central house-audio server owns the library, queue, current position, and synchronized stream.

A direct SMB-on-ESP32 test was considered early, before the architecture pivoted to Snapcast-style synchronized renderers. That test is now optional and low priority because it does not validate the production data path.

## Phase 1: serial-only Snapcast client proof — PASS

Phase 1 has been completed successfully on a **Seeed Studio XIAO ESP32-S3** with no DAC or amplifier attached.

### Test firmware

The working proof uses ESPHome with ESP-IDF and the external Snapcast component:

```text
github://c-MM/esphome-snapclient@main
```

The test connected directly to the permanent Pi Snapserver at port 1704. The ESPHome component currently requires an explicit `hostname` value because leaving it omitted produces an invalid default-domain value in current ESPHome validation.

The proven I2S pin assignment is:

```text
PCM5102A LCK -> XIAO D3 = GPIO4
PCM5102A BCK -> XIAO D4 = GPIO5
PCM5102A DIN -> XIAO D5 = GPIO6
```

Important: the XIAO's printed `D` labels are not the same numbers as the ESP32 GPIO numbers. The first audible test was initially wired one physical pin off because the firmware GPIO numbers were mistaken for the board's printed D-labels. Correcting the mapping above immediately produced audio.

### What was proven

The XIAO successfully:

- booted the ESPHome/ESP-IDF firmware
- joined the home Wi-Fi
- connected to the permanent Snapserver
- sent the Snapcast hello message
- negotiated a FLAC stream at `48000:16:2`
- filled a **1000 ms** latency buffer
- changed mute state when playback state changed
- stayed alive during sustained playback
- continuously received and acknowledged the real audio stream

Representative serial output:

```text
netconn connected
netconn sent hello message
Buffer length:  1000
Latency:        0
Mute:           0
Setting volume: 100
fLaC sampleformat: 48000:16:2
latency buffer full
```

When a real song started, the client reported:

```text
Unmute
```

### Direct sustained-stream proof

The original acceptance goal was not merely "TCP connected"; it was to prove that real audio payload continued flowing while the song played.

That was verified from the permanent Snapserver host with:

```bash
watch -n 1 'ss -tin sport = :1704'
```

For the XIAO connection, over one 59-second sample:

```text
bytes_sent:    7,681,191 -> 13,974,143
bytes_acked:   approximately tracked bytes_sent
data_segs_out: 6,904 -> 12,247
```

Delta:

```text
6,292,952 bytes
5,343 TCP data segments
~0.85 Mbit/s sustained
```

That traffic volume, continuously acknowledged by the XIAO while the song played, proves actual stream reception rather than only handshake/control traffic.

**Phase 1 no-DAC receive test: PASS.**

### Antenna result

The first test was accidentally performed with no external antenna connected. RSSI was roughly `-85 dBm` and the client showed Wi-Fi roam activity plus mute/unmute wobble.

After installing the XIAO's 2.4 GHz antenna, signal improved to roughly:

```text
-37 dBm
```

The client then connected cleanly and filled the Snapcast latency buffer. The approximately **48 dB** improvement makes the antenna mandatory for production installations using this XIAO variant.

## Synchronization direction

The renderer uses an existing ESP32 Snapcast-client implementation rather than inventing a synchronization protocol. The goal is timestamped/buffered playback with clock correction, not several independent decoders attempting to seek to approximately the same position.

The ESP32-S3 has now been proven capable of receiving and decoding the production Snapcast stream, producing clean analog audio through PCM5102A, and synchronizing audibly with a second independent node.

## Phase 2: one real audio node — AUDIBLE PLAYBACK PROVEN

A PCM5102A line-level DAC has now been connected to the XIAO and **real audible playback is working** from the permanent house-audio stack.

Proven path:

```text
Pi local music -> MPD -> Snapserver FLAC 48000:16:2
-> Wi-Fi -> XIAO ESP32-S3 -> I2S -> PCM5102A -> analog audio
```

The working physical I2S mapping is:

```text
LCK -> D3 (GPIO4)
BCK -> D4 (GPIO5)
DIN -> D5 (GPIO6)
```

Snapserver was returned to its normal FLAC transport after a temporary PCM troubleshooting test, and the running server confirmed `codec=flac`.

Phase 2 behavior now proven:

- automatic join to an already-running song;
- hard power-cycle recovery with real audio attached;
- same-song rejoin after more than ten seconds powered off while the house session remained active;
- about six seconds observed from plug-in to audible rejoin on current hardware.

Still under investigation:

- occasional few-second audio dropouts affecting one renderer or the other during otherwise synchronized playback;
- brief Wi-Fi/server interruption recovery has not yet been isolated from that dropout investigation;
- no-objectionable-stutter acceptance remains open until the intermittent dropout cause is understood.

## Phase 3: synchronization proof — PASS

A second XIAO ESP32-S3 + PCM5102A renderer was built as a functional clone of the first node.

The real acceptance test passed:

1. two independent renderers connected to the same Snapserver stream;
2. both produced real analog audio at the same time;
3. the nodes fed very different downstream amplifier/speaker systems;
4. the outputs were **audibly synchronized**;
5. no objectionable echo or phasing was heard during the test.

This proves the synchronization architecture at the level that matters: two separate Wi-Fi clients, clocks, DACs, amplifiers, and speakers can render one coherent house session.

Fixed per-node latency compensation remains available later if a particular DAC/amplifier path introduces a repeatable offset, but none was required for this proof.

## Hardware direction

After the prototype path is proven, design one generic PCB that can be installed repeatedly.

Likely functions:

- ESP32-S3 module
- required 2.4 GHz antenna arrangement
- power regulation
- I2S DAC / line-level output
- headers or pads for stereo output
- optional mono-summing provision
- spare GPIO/test pads

Do not freeze the PCB until the ESP32 client, DAC choice, power arrangement, and firmware stack have been proven together.

## Related projects

- [house-audio-server](https://github.com/oolah10293/house-audio-server) — Raspberry Pi playback/session authority and Snapserver
- [smb-music-player](https://github.com/oolah10293/smb-music-player) — Android player/controller
- [smb-player-pc](https://github.com/oolah10293/smb-player-pc) — Windows player/controller

## Status

**Phase 1 is complete. Phase 2 audible playback and hard-power rejoin behavior are proven. Phase 3 two-renderer audible synchronization is PASS.** Two XIAO ESP32-S3 + PCM5102A nodes now produce synchronized analog audio from the permanent house stream. The main renderer-side open item is reliability: diagnose the occasional few-second single-node dropout and confirm clean recovery once its cause is known.

## Multi-node field findings

### Appliance behavior

The renderer now behaves like the intended old-radio appliance:

- power the radio/node on and the server starts a fresh randomized default after a completed session, or resumes/joins an unfinished active session;
- if the house session is already playing, the renderer joins the current song instead of restarting it;
- if the node is hard-powered off for more than ten seconds and then restored while the house session remains active, it rejoins that same song;
- one observed power-on/rejoin reached audible output in about six seconds.

A separate server-policy bug was found when both nodes had been off long enough for MPD to be paused. Both renderers returned healthy, but v0.5.0 intentionally left the paused session silent. `house-audio-server` v0.5.1 now treats passive-radio arrival as Play intent and resumes the retained queue. Both nodes started immediately after that server update. No ESP32 firmware change was required for that fix.

### Two-node identity/configuration

The second renderer uses the same proven firmware/hardware pattern as the first. The Snapcast/audio/I2S settings can remain the same. Each XIAO's hardware MAC gives Snapserver a distinct client id automatically; unique friendly names are recommended for human-readable diagnostics but are not required for synchronization.

The proven I2S wiring on both nodes remains:

```text
LCK -> D3 / GPIO4
BCK -> D4 / GPIO5
DIN -> D5 / GPIO6
```

Snapserver distinguishes the nodes by their unique client/MAC identity.

### Antennas

Both active XIAO S3 renderers have their external 2.4 GHz antennas installed. The first node's earlier no-antenna test had shown roughly -85 dBm and obvious Wi-Fi instability; with the antenna attached it improved to roughly -37 dBm. The current intermittent dropout investigation therefore is **not** explained simply by a missing antenna.

### Intermittent few-second dropout

During two-node playback, an occasional short silence of a few seconds has been heard on one node or the other. The interruption is not necessarily frequent, and the system recovers automatically.

No root cause has been assigned yet. Possibilities still include:

- per-node Wi-Fi/TCP timing stall;
- Snapcast client/decoder buffer behavior;
- I2S/audio-output path;
- another client-specific issue.

The server-side v0.6.0 diagnostics recorder now captures Snapcast `lastSeen` stalls/recovery, connected/present/audible transitions, and global Snapserver stream-state changes. The next field occurrence should be correlated against `GET /diagnostics`. If those server-side signals remain clean during an audible dropout, the next instrumentation belongs inside the ESP32 decoder/buffer/I2S path.

Tracking: [Issue #3 — intermittent few-second single-node audio dropouts](https://github.com/oolah10293/house-audio-esp32/issues/3).

### Subjective audio-quality observation

Using the same downstream amplifier, speakers, and analog cable, the PCM5102A/Snapcast source path was subjectively reported as noticeably cleaner than the generic Bluetooth receiver board it replaced, especially in high-frequency clarity/presence and low-level mix detail.

This is an informal listening observation, not a lab measurement, but it is useful practical evidence that the tiny renderer is not merely convenient; its analog output quality is good enough to expose detail that the prior receiver path appeared to obscure.


### v0.6.1 server-boundary proof

A new "powered renderer but no music" event was captured with the server diagnostics and proved **not** to be an ESP32/DAC failure.

The active S3 was:

- connected to Snapserver;
- fresh/present;
- unmuted;
- otherwise healthy.

The actual state was upstream: MPD had finished the prior no-renderer session under `single oneshot` and landed **paused at 0.0 seconds on the next queued track**, leaving the Snapserver stream idle.

`house-audio-server` v0.6.1 recognized that MPD boundary state and restored audible output by resuming the retained queue. That happened on the permanent Pi with no renderer firmware, wiring, or hardware changes. **The old-queue resume choice is now superseded by the authoritative session rule:** after completed drain the session is over; next passive power-on starts the configured MP3s/Rap default with a new random shuffle. Server v0.6.2 implements/tests that correction and is now running on the permanent Pi. Initial radio results are recorded below. Return before the final song ends still preserves the existing session, and ordinary paused sessions remain resumable.

This matters for renderer troubleshooting: a silent but healthy/present node is not automatically a Wi-Fi, decoder, DAC, or I2S fault. Check the authoritative MPD/session state before changing ESP32 hardware.


### v0.6.2 radio power-cycle results — 2026-09-29

- About **10 seconds unplugged**: the radio returned to the **same song**.
- About **five minutes unplugged**: powering the radio on started a **different new song**.
- The supplied `/session` response confirms server **0.6.2**, passive default **MP3s**, one present renderer, no pending stop, and `lastAction: pending_stop_cancelled_renderer_returned`.

The snapshot verifies the early-return path. The long-absence listening result is consistent with fresh-session startup; a captured fresh-start action and a manually selected CD/Rap queue-to-default comparison were not supplied. Full details are in the server API docs. Server v0.7.0 now implements persisted MP3s/Rap selection via GET/POST `/settings` in source/tests (42 local tests pass; Pi installation pending). The Android selector and controller presence remain pending. Saving the default never interrupts a playing radio; it applies at the next fresh passive session. This is a server-release validation update, not a new ESP32 firmware build.
