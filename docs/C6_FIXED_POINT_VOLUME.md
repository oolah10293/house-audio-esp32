# Motorola C6: fixed-point Snapcast client-volume fix (candidate, unbuilt)

## Scope
This **only** changes the ESPHome Snapclient used by the **Motorola Radio XIAO ESP32-C6**. The HOUSE server, Android HOUSE Music app, other regular S3 renderers, and Bose subwoofer are unchanged.

## Field evidence (2026-10-17)
- C6 firmware successfully boots, joins Wi-Fi and plays HOUSE audio.
- Snapdroid client volume at 100%: normal playback.
- Reduce the Motorola client volume: playback becomes extremely choppy.
- Restore client volume to 100%: playback immediately returns to normal.
- Occasional baseline dropouts occur on both C6 and S3 even at 100%; **this patch does not claim to fix those**.

## Root-cause hypothesis
The exact pinned upstream `luar123/snapclient` revision `774268009d1a6c3d1664fad533409a22e5a41e83` processes PCM at `components/dsp_processor/dsp_processor.c` (`dspfStereo`). At exactly 100%, it skips volume scaling. Below 100%, it multiplies *every sample* by a floating-point `dynamic_vol`. The single-core ESP32-C6 has no hardware FPU. Additional work inside the audio path is a compelling explanation for the volume-dependent underruns, but has not yet been established by CPU timing or on-device instrumentation.

## Minimal patch
- Original interface, Snapcast protocol, decoder, I²S pins and buffers are unchanged.
- In the stereo soft-volume path, load one **atomic Q16.16 gain** per 16-frame block and multiply each signed 16-bit L/R sample using 32-bit integers.
- Compute Q16.16 from incoming floating-point volume **once per update**, not once per sample.
- Preserve full-volume pass-through; preserve separate left and right channels.
- Preserve upstream floating-point behavior for other DSP flows; this patch only optimizes stereo soft volume.
- Worst-case PCM multiplication fits signed int32 (`-32768 × 65536 = INT32_MIN`). Repack signed sample bits explicitly, never by adding signed shifted values.
- The gain is atomic because the control and audio paths can execute asynchronously.

The patched DSP is copied from `luar123/snapclient` at the pinned revision and published in `components/dsp_processor/` with the upstream GPLv3 license. The ESPHome wrapper in `components/snapclient/` is copied from `luar123/esphome@snapclient2` and replaces **only** the `dsp_processor` IDF dependency with the patched `components/dsp_processor` folder in this repository. All other IDF component dependencies remain on the unchanged pinned upstream revision.

**Immutable dependency pins:** The C6 YAML uses ESPHome-wrapper commit `19e4679b1b8d96c5416f94836d54300e6f0a25d8`. That wrapper selects patched DSP commit `d135a2735cb59378f732bc1373607633491f1aaf`.

## Configuration
See `firmware/motorola-radio-c6-fixed-volume.yaml`. Copy its changes into the running Motorola YAML when ready to test; no IP address was hard-coded. The physical D3/D4/D5 DAC connections do not move.

## Validation and limitations
- **Done:** Source inspected, isolated software-volume path changed, static arithmetic vectors checked at 0–100% including signed full-scale values; maximum difference is one PCM count from the floating-point reference.
- **Not done:** No ESPHome/ESP-IDF firmware compile, installation, release packaging or on-device validation was authorized or performed.
- A full compile may expose ESPHome or ESP-IDF dependency integration issues; those cannot be ruled out from inspection alone.
- After a separately authorized build/flash: verify playback at 100%, 50%, 20%, 1%, and mute; then leave Motorola playing for a normal listening session and check for disconnects/underruns. Confirm S3 nodes are untouched.
- If volume-dependent choppiness continues, capture C6 audio, CPU/heap or queue diagnostics; don't automatically attribute all dropouts to the same cause.
