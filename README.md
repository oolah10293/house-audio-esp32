# House Audio ESP32

ESP32-S3 synchronized audio renderer firmware for the whole-house music system.

These nodes are intended to hide inside vintage radios, stereos, powered speakers, or small standalone boxes and make them outputs for the single house playback session.

**Companion correction status — 2026-09-30:** [Android v0.4.1](https://github.com/oolah10293/smb-music-player/blob/main/docs/RELEASE_0.4.1.md) implements the first-phone-pass Tailscale/routing, conditional playlist auto-unmute and lower-strip mute-control fixes. Server v0.8.2 is installed and healthy. v0.4.0 proved HOUSE track adoption with Tailscale off; v0.4.1 Tailscale-on behavior and phone/S3 synchronization still need acceptance. **No ESP32 firmware or wiring change is required.**

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

The snapshot verifies the early-return path. The long-absence listening result is consistent with fresh-session startup; a captured fresh-start action and a manually selected CD/Rap queue-to-default comparison were not supplied. Full details are in the server API docs. Server v0.7.0 now implements persisted MP3s/Rap selection via GET/POST `/settings`, and it is installed and field-proven on the permanent Pi. Changing the default from MP3s to Rap did not interrupt the song already playing. After the final S3 stayed off for about ten minutes and the old session drained, the next S3 power-on started a fresh Rap session (first observed track: Ludacris — *Southern Hospitality*). The Android selector remains pending; controller presence is implemented/tested in server v0.8.0, awaiting Pi validation. This is a server-release validation update, not a new ESP32 firmware build.


### v0.7.0 passive-default field result

A real S3 was used to validate the server's persisted passive-default behavior:

- server default changed from `MP3s` to `Rap`;
- current playback was unaffected;
- S3 was powered off for roughly ten minutes so the prior session could complete;
- on power-up, the S3 received a fresh Rap session;
- first observed track: Ludacris — *Southern Hospitality*.

No ESP32 firmware change was involved. This confirms that the renderer correctly follows the server-owned fresh-session/default-folder policy.

### Server v0.8.0 controller policy — source/tests complete

Server v0.8.0 adds controller leases and muted-phone session handling, with 73 passing tests and CI. It is installed on the permanent Pi: health passes, and a real S3 is correctly classified as one present/audible passive renderer with zero controllers. Physical controller pause/resume/expiry checks remain pending.

A muted controller remaining after the last audible radio leaves holds an exact paused session. A radio returning resumes it. If that last controller leaves the automatic pause, the session ends without advancing; the next radio starts the configured default with a fresh shuffle. Leaving during active playback and returning before the final song ends still preserves the existing session.

Background controllers renew every five seconds and expire after fifteen seconds without a heartbeat. Phone renderers retain their controlling-device classification across disconnects/service restarts and cannot masquerade as passive radios. No ESP32 firmware change is required for these server rules.

### Server v0.8.1 restart boundary — deployed, already-present-radio path proven

A control-service restart now ends the previous listening session: stop/clear MPD and reset leftover playback modes before accepting new playback. A passive S3 already present or arriving later starts the currently saved MP3s/Rap default with a new shuffle. Controller reconnect alone stays idle. The saved default and phone-renderer ownership survive; old queue/progress and live leases do not.

A radio returning before the final song ends during the same server process still continues the existing session unchanged. MPD/Snapserver connection recovery within that process does not trigger another startup reset. No ESP32 firmware change is required.

85 tests and CI passed for server v0.8.1, which is installed on the permanent Pi. The update restarted the service while one S3 stayed powered: startup reached ready, the saved Rap default survived, and a fresh randomized Rap session began with `lastAction: started_default_session`, one passive/audible renderer, zero controllers, and no auto-pause or pending drain. The all-radios-off restart variant and physical controller transitions remain pending.


### Android v0.4.0 / server v0.8.2 integration checkpoint

The first Android HOUSE backend/receiver and approved Browser polish are implemented and the final `9c89b24` APK is delivered. Server v0.8.2 is installed and healthy. The phone has already proven initial HOUSE state adoption with Tailscale off after MPD's LAN listener was enabled, but enabling Tailscale stops app updates while normal browser traffic can still reach the Pi. The next Android correction separates physical-home qualification from ordinary packet routing, applies the clarified conditional auto-unmute rule for playlist starts, and moves Mute/Unmute into the lower Media3 control strip. A muted phone stays muted when another house output was already audibly playing, but auto-unmutes when it initiates playback from an otherwise inaudible state. After that, resume the phone/S3 synchronization and muted-controller lifecycle checkpoint. No ESP32 firmware or wiring change is part of this iteration.


### Android/Tailscale field finding — no ESP32 change

The first Android v0.4.0 HOUSE test did **not** identify an ESP32/Snapcast-node defect.

Observed:

- the Pi/server and S3 continued operating;
- Android HOUSE state adoption worked with Tailscale off;
- enabling Tailscale stopped Android app updates;
- the same phone could still reach the Pi HTTP health endpoint through normal browser traffic.

The correction is entirely on the Android client networking side: physical non-VPN LAN presence determines HOUSE, while normal Android routing carries control/audio traffic. The S3 firmware remains unchanged for the next phone synchronization test.

### Android v0.4.1 — corrections implemented, phone acceptance next

The [v0.4.1 release record](https://github.com/oolah10293/smb-music-player/blob/main/docs/RELEASE_0.4.1.md) tracks build evidence and exact artifacts. Physical home-network routes now qualify HOUSE while normal Android routing carries control/audio traffic. Song/PLAY LIST preserves a muted phone if another output was already audible; pause/stop or otherwise inaudible starts auto-unmute it. Mute/Unmute is an icon inside the lower Media3 control strip.

Keep the current S3 firmware and installed Pi v0.8.2. Next run [Tailscale-on launch/toggle, local output and phone/S3 acceptance](https://github.com/oolah10293/smb-music-player/blob/main/docs/HOUSE_VALIDATION.md); no Android audible synchronization or controller-lifecycle pass is claimed yet. Home/away same-song handoff remains subsequent Android work.


### Android v0.4.1 phone/S3 field result

The Android correction build now has partial real-device acceptance:

- HOUSE works with Tailscale connected.
- The phone's lower-strip output control is accepted.
- A muted phone can change the shared playlist while an S3 is already audible without unmuting itself.
- **Phone/S3 synchronization currently fails acceptance:** the phone was observed about **1 second behind** the S3.

This does **not** invalidate the existing S3-to-S3 synchronization proof; that remains PASS. The one-second phone lag is a separate Android/Snapcast-renderer integration result and has not yet been diagnosed.

A new Android Bluetooth-output policy was also approved: Bluetooth route connect/disconnect drives the phone's local HOUSE output state, while the S3s and Pi continue following the existing shared-session policy. No ESP32 firmware change is implied.
