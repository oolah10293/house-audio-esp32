# House Audio ESP32

ESP32-S3 synchronized audio renderer firmware for the whole-house music system.

These nodes are intended to hide inside vintage radios, stereos, powered speakers, or small standalone boxes and make them outputs for the single house playback session.

## Core production behavior

In the finished system, an ESP32 node is a **renderer**, not an independent music player.

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

## Production SMB rule

The finished ESP32 node should **not** need to mount the music share or build playlists. The central house-audio server is expected to own the music library, queue, current position, and synchronized stream.

However, the **first hardware test intentionally does use SMB directly** because the house-audio server does not exist yet. That is a feasibility test, not the final architecture.

## Phase 0: direct SMB feasibility test — FIRST TEST

The very first test uses only:

- one ESP32-S3
- USB cable
- the existing home Wi-Fi
- the existing SMB music share
- no DAC
- no amplifier
- no house-audio server

The S3 should:

1. connect to Wi-Fi
2. connect/authenticate to the real SMB share
3. list the target music directory
4. open a real MP3 or FLAC file
5. read the **entire file** sequentially in chunks
6. discard the bytes after reading
7. report the result over USB serial

Useful serial diagnostics:

- SMB connection/authentication state
- directory-listing result
- selected file/path
- file size
- total bytes read
- elapsed time and average throughput
- read errors/retries
- optional CRC32/checksum
- final PASS/FAIL

This answers the immediate question: **can the ESP32-S3 reliably access the real SMB music library?**

## Phase 1: synchronized-stream proof

After Phase 0 passes, stand up the minimum house-audio/synchronization server and repurpose the same S3 for a serial-only renderer test.

Acceptance criteria:

- connects to Wi-Fi
- discovers or reaches the house audio/sync server
- completes stream/client negotiation
- receives real stream packets continuously
- reports packet/byte counts, timing/buffer state, failures, and reconnects over serial

No DAC is required yet.

## Synchronization direction

Use an existing Snapcast-compatible ESP32 client if it proves reliable on ESP32-S3. The goal is timestamped/buffered playback with clock correction, not several independent decoders attempting to seek to approximately the same position.

The exact client/transport implementation remains open until tested on real hardware.

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

- [house-audio-server](https://github.com/oolah10293/house-audio-server) — central playback/session authority and synchronized stream
- [smb-music-player](https://github.com/oolah10293/smb-music-player) — Android player/controller
- [smb-player-pc](https://github.com/oolah10293/smb-player-pc) — Windows player/controller

## Status

Waiting for the first spare ESP32-S3 to begin **Phase 0: direct SMB access and full-file read over serial**.
