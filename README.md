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

## Planned audio path

Baseline direction:

```text
Wi-Fi -> ESP32-S3 -> Snapcast client -> I2S -> external DAC -> existing amplifier/stereo -> speaker
```

A PCM5102A-class line-level DAC is the current likely direction. Existing amplifiers and analog volume controls should be preserved where practical.

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

Dummy I2S pins were configured so the decoder/player path could run without a physical DAC attached:

```text
LRCLK: GPIO4
BCLK:  GPIO5
DOUT:  GPIO6
```

These pins are not yet a final production pin assignment.

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

The ESP32-S3 has now been proven capable of receiving and decoding the production Snapcast stream. Audible synchronization still requires the DAC/audio-output phase and then a second node.

## Phase 2: one real audio node — NEXT

Add an I2S line-level DAC, initially PCM5102A-class, and verify:

- real clean continuous audio
- correct channel handling / mono summing where required
- automatic join to an already-running song
- reconnect after Wi-Fi/server interruption
- hard power-cycle recovery

The immediate next hardware step is to connect a PCM5102A to the proven I2S path and hear the stream.

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

**Phase 1 is complete.** The XIAO ESP32-S3 has been proven as a real Snapcast FLAC receiver on the permanent house-audio server, including sustained payload transfer. The next step is **PCM5102A/I2S audio output**.