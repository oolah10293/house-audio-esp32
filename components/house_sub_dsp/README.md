# HOUSE subwoofer DSP — corrected PR #14389 integration

This version supersedes the first add-on, which mistakenly retained c-MM/esphome-snapclient. It requires the newer ESPHome Snapclient PR #14389, pinned at luar123/esphome commit 4d3280bd35fdd970e628a22197f19ab0fded1a39. That wrapper selects luar123/snapclient core 774268009d1a6c3d1664fad533409a22e5a41e83, including its newer player/sync/timefilter work. The reliability baseline remains under field evaluation; this does not assert all dropouts are fixed.

Scope: (L+R)/2 mono, followed by a fixed 90 Hz fourth-order Linkwitz-Riley low-pass, duplicated into both PCM5102A slots. Existing Bose startup/attenuation, D3/D4/D5 I2S wiring, server and app remain unchanged. No automatic standby or runtime controls added.

The PR is a media_player platform, not a top-level snapclient component. Its DSP entry point is int dsp_processor_worker(void *pcm_chunk, const void *settings), unlike the old three-argument raw-buffer ABI. The adapter uses the actual upstream headers and a compile-time signature assertion. ESPHome final validation rejects the old implementation and unreviewed core revisions.

GNU ld --wrap interposes on this two-argument entry point and calls the original software-volume worker exactly once per fragment before filtering. There is no replacement decoder/player, no old c-MM dependency, no mDNS override, no extra audio queue and no timestamp changes. The native biquad/filter is the same tested arithmetic from the first addition. Coefficients for the normal 48 kHz stream are prepared at setup, and activity/error logging is deferred to the ESPHome loop rather than formatted in the audio task. Unsupported PCM formats are silenced rather than played unfiltered. The current supported data contract is 16-bit stereo, including the HOUSE 48000:16:2 stream.

The pinning and native ABI checks deliberately prevent another silent fallback to the wrong baseline. The original known-good firmware file on main is not changed.

Expected log once real PCM has been processed:

    PCM DSP ACTIVE: PR14389, mono, 90.0 Hz LR4, 48000 Hz stream

Tests cover LR4 frequency response, channel equality, sum headroom, saturation, reset and chunk continuity, plus the new chunk/settings ABI, fragment handling, single volume application, timestamp preservation, invalid-format rejection, exact source/core pins, successful ESPHome compilation and final ELF call-site interposition. The new electrical low-pass adds frequency-dependent phase/group delay; unchanged Snapcast scheduling does not imply unchanged acoustic phase. Hardware listening remains required.
