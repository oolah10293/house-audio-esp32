# House Audio ESP32

ESP32-S3 synchronized audio renderer firmware for the whole-house music system.

These nodes are intended to hide inside vintage radios, stereos, powered speakers, or small standalone boxes and make them outputs for the single house playback session.

## Core behavior

An ESP32 node is a **renderer**, not an independent music player.

On power-up it should:

1. boot
2. join the home Wi-Fi
3. discover the house-audio service on the local LAN
4. connect to the synchronized audio stream
5. fill its timing/audio buffer
6. begin output at the **current house playback timestamp**

If music is already halfway through a song when a node turns on, the node joins at that point. It does not restart the song and it does not create its own queue.

If the server is unavailable, the node should simply keep retrying. No phone interaction should be required.

## Hard power-off is intentional

Many nodes will live inside existing radios/stereos whose original power switch physically removes power. That behavior is a design requirement, not a fault condition.

The ESP32 firmware must tolerate abrupt power removal with no shutdown sequence. Turning a radio off should make that output disappear immediately while the central house session continues elsewhere.

## Planned audio path

Baseline direction:

```text
Wi-Fi -> ESP32-S3 -> I2S -> external DAC -> existing amplifier/stereo -> speaker
```

A PCM5102A-class line-level DAC is the current likely direction, but the exact DAC is not yet locked. Existing amplifiers and analog volume controls should be preserved where practical.

For mono equipment, stereo DAC outputs must be summed through resistors rather than tied directly together.

## No SMB on the node

The ESP32 should **not** mount the music share or build playlists. The central house-audio server owns the music library, queue, current position, and synchronized stream. Keeping SMB out of the endpoint substantially reduces firmware complexity.

## Synchronization direction

Use an existing Snapcast-compatible ESP32 client if it proves reliable on ESP32-S3. The goal is timestamped/buffered playback with clock correction, not several independent decoders attempting to seek to approximately the same position.

The exact client/transport implementation remains open until tested on real hardware.

## Phase 1: no DAC required

The first proof can be done with only an ESP32-S3 and USB serial.

Acceptance criteria for the first test:

- connects to Wi-Fi
- discovers or reaches the house audio/sync server
- completes stream/client negotiation
- receives real stream packets continuously
- reports useful serial diagnostics such as connection state, packet/byte counts, timing/buffer state, and failures/reconnects

This proves the difficult network/client path before buying or wiring audio hardware.

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

After the breadboard/prototype path is proven, design one generic PCB that can be installed repeatedly. The board should target multiple identical deployments rather than one-off wiring.

Likely functions:

- ESP32-S3 module
- power regulation
- I2S DAC / line-level output
- headers or pads for stereo output
- optional mono-summing provision
- spare GPIO/test pads

Do not freeze the PCB until the ESP32 client, DAC choice, power arrangement, and firmware stack have been proven together.

## Related projects

- [house-audio-server](https://github.com/oolah10293/house-audio-server) — central playback/session authority and synchronized stream
- [smb-music-player](https://github.com/oolah10293/smb-music-player) — Android player/controller
- [smb-player-pc](https://github.com/oolah10293/smb-player-pc) — Windows player/controller

## Status

Waiting for the first spare ESP32-S3 to begin the serial-only stream-reception proof.
