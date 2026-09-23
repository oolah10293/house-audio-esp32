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
/mnt/sharedrive/Shared Music
```

That path is **server-side only**. The ESP32 does not browse or mount it.

The planned production path is:

```text
Raspberry Pi local files -> MPD -> Snapserver -> ESP32-S3 Snapcast client -> I2S DAC -> amplifier/stereo
```

The first ESP32 proof should use the same Snapserver instance intended to remain in production. No disposable proof server is planned.

## Hard power-off is intentional

Many nodes will live inside existing radios/stereos whose original power switch physically removes power. That behavior is a design requirement, not a fault condition.

The ESP32 firmware must tolerate abrupt power removal with no shutdown sequence. Turning a radio off should make that output disappear immediately while the central house session continues elsewhere.

## Planned audio path

Baseline direction:

```text
Wi-Fi -> ESP32-S3 -> Snapcast client -> I2S -> external DAC -> existing amplifier/stereo -> speaker
```

A PCM5102A-class line-level DAC is the current likely direction, but the exact DAC is not yet locked. Existing amplifiers and analog volume controls should be preserved where practical.

For mono equipment, stereo DAC outputs must be summed through resistors rather than tied directly together.

## SMB rule

The ESP32 node should **not** mount the music SMB share or build playlists in the planned production architecture. The central house-audio server owns the library, queue, current position, and synchronized stream.

A direct SMB-on-ESP32 test was considered early, before the architecture pivoted to Snapcast-style synchronized renderers. That test is now optional and low priority because it does not validate the production data path.

## Phase 1: serial-only Snapcast client proof — FIRST TEST

The first meaningful hardware test should use one ESP32-S3 with USB serial and no DAC.

Prerequisite: the Raspberry Pi has the permanent MPD -> Snapserver path running far enough to produce a real Snapcast stream.

The S3 should:

1. connect to Wi-Fi
2. discover or connect to the Pi's Snapserver
3. complete Snapcast client/stream negotiation
4. continuously receive real stream data
5. report useful diagnostics over USB serial

Useful diagnostics include:

- Wi-Fi connection state
- server discovery / address
- stream connection state
- codec / stream parameters
- packet and byte counters
- buffer / timing state where available
- reconnect attempts and failures
- final PASS/FAIL indication for a sustained receive test

This validates the part that matters for the finished nodes: **can this exact ESP32-S3 board operate reliably as a synchronized Snapcast renderer?**

## Synchronization direction

Prefer an existing ESP32 Snapcast-client implementation rather than inventing a synchronization protocol. The goal is timestamped/buffered playback with clock correction, not several independent decoders attempting to seek to approximately the same position.

The exact ESP32 client implementation and configuration remain open until tested on the real S3 hardware.

## Phase 2: one real audio node

Add an I2S DAC and verify:

- clean continuous audio
- automatic join to an already-running song
- reconnect after Wi-Fi/server interruption
- hard power-cycle recovery

## Phase 3: synchronization proof

Add a second renderer and perform the real acceptance test:

1. music is already playing in one room
2. power on another node
3. second node joins the same song at the current point
4. walk between rooms
5. no objectionable echo or phasing is audible

Fixed per-node latency compensation can be added later if particular DAC/amplifier paths introduce repeatable offsets.

## Hardware direction

After the prototype path is proven, design one generic PCB that can be installed repeatedly.

Likely functions:

- ESP32-S3 module
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

Waiting for the first spare ESP32-S3 and the first permanent Pi MPD/Snapserver configuration to begin **Phase 1: serial-only Snapcast client reception proof**.
