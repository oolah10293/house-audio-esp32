# Proposal: reusable House Music audio-input nodes (2026-11-11)

**Status: design proposal, not implementation authorization.** This documents the owner's requirement that input hardware not be a feature hard-coded to a particular Seeed XIAO ESP32-S3. No firmware, server change, release, installer, or package is authorized by this note.

## Verified baseline

- `house-audio-server` at main `aa5e83cbd270cf737740cf0e96f3815b1b012b6d`: MPD decodes the currently chosen program, sends stereo PCM at `48000:16:2` to `/tmp/snapfifo`, and Snapserver synchronizes output to renderer nodes. The server's radio pathway already selects a stream URL through MPD using a stop/clear/add/play sequence.
- `house-audio-esp32` at main `c539bf70b7f925e30a32eee09357f30bf4d50299`: listening nodes independently join the **one shared house stream**; renderer nodes do not own the MPD queue or source.
- `feature/cdda-arduino-ide`: the user successfully compiled and uploaded `NetworkCDProbe` in **Arduino IDE with ESP32 core 2.0.14** using the esptool `--no-stub` override. A user screenshot of the Wi-Fi status page shows the Lite-On USB drive's CD TOC with 11 audio tracks, 0 data tracks. **No audio sectors or network audio transport have yet been verified.**

## Requirement

Create **source nodes** analogous to existing renderer/listening nodes:

- No source bound permanently to a specific ESP32 board, IP address, attached optical drive, or capture method.
- A source device identifies itself to the central server with stable `sourceId`, friendly name, capabilities, type, and ready/playing/error status. The server tracks source presence and timeout.
- Sources may be USB audio-CD devices, analog stereo ADC inputs, and future input technologies. Individual nodes can have input, output, or both roles when hardware allows.
- Hardware-specific capture drivers are separate from the common Wi-Fi transport, device identity, control signaling, and buffering contract.
- House Music clients can enumerate and deliberately select an available source. **Simply powering up a source must not steal playback** (proposed behavior; confirm before implementing). Radio/library snapshot and return semantics should be revisited before source switching.
- There is one selected house program, reaching existing Snapcast listener nodes in sync. Existing output nodes should require no firmware changes for basic playback.

## Candidate integration approach (not verified)

CD or analog ADC captures digital PCM and streams to a **Pi-side source gateway**. The gateway publishes the selected live audio as an MPD-decodable input, e.g. local HTTP audio stream, so MPD remains the common audio conversion and stream-selection point before the established FIFO/Snapserver chain. Alternative: a carefully controlled central PCM source multiplexer upstream of Snapserver. Choose only after testing live MPD input and required buffering/latency against a real CD.

A source protocol would distinguish:

- **Discovery/presence:** stable identity, human-readable name, kind, capabilities, current readiness; source connection and heartbeat distinct from listener presence/auto-start counts.
- **Audio stream:** documented PCM sample format, clock rate, channel layout, frames/sequence or timebase, transport and connection lifecycle. CD is 44.1 kHz 16-bit stereo; current Snapserver feed is 48 kHz 16-bit stereo, so rate conversion must occur in an appropriate layer.
- **Optional controls:** CD play/pause/track selection and metadata where the source supports it; line-in may only support start/stop/gain; capabilities advertised instead of assumed.
- **Failure policy:** source disappearance, reconnect, pause and selected-source restoration decided centrally; failure never silently changes the selected library queue.

## Smallest useful next step

1. Define one source identity and PCM audio contract usable by both a CD transport and later analog ADC.
2. Prove one input's PCM arrives at the Pi and can be heard through the unchanged Snapcast renderer path.
3. Add source discovery/selection and lifecycle to House Music, then CD transport controls and eventual analog ADC adapter.

**Outstanding/untested:** actual SCSI READ CD extraction, PCM timing and sample-rate conversion, MPD live-stream acceptance and buffering, network continuity, input-to-output latency (important for TV), device registration/heartbeat behavior, source-switch and playlist-resume semantics, output sync.

Preserve the user's existing Arduino IDE workflow and ESP32 core 2.0.14 (selected for separate working CRT composite visualizer) unless separately authorized to change it. **Do not build, package, deploy or release as a consequence of this document.**
