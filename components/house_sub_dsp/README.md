# HOUSE subwoofer DSP — Home Assistant tuning controls

This component keeps the accepted newer Snapclient PR #14389 baseline pinned at
`luar123/esphome` commit `4d3280bd35fdd970e628a22197f19ab0fded1a39`
and its Snapclient core
`774268009d1a6c3d1664fad533409a22e5a41e83`.

The renderer always performs `(L+R)/2` mono summing and can accept these
runtime settings without recompiling:

- fourth-order Linkwitz-Riley low-pass;
- optional fourth-order Linkwitz-Riley low-cut/high-pass;
- 0/180-degree polarity inversion;
- crossover bypass while retaining mono summing and the selected phase.

The settings cross from ESPHome's main loop into the decoder task through
atomics. Filter coefficients and state change only in the audio task at a PCM
chunk boundary. No transport buffers, timestamps, queue lengths, I2S pins,
Snapcast synchronization code, or Bose control-bus code are replaced.
Snapcast software volume still runs exactly once per fragment before the node
DSP.

The companion YAML exposes five native Home Assistant tuning entities:

- **Bose Hardware Attenuation:** 0–40; higher is quieter; default 12.
- **Sub Low-Pass:** 60–160 Hz; default 90 Hz.
- **Sub Low-Cut:** 0, 10, 20, 30, 40, or 50 Hz; zero disables it.
- **Sub Phase 180 Degrees:** polarity inversion.
- **Sub Crossover Bypass:** bypasses the HP/LP filters only.

All values restore after reboot. Slider actions use restart-mode scripts with a
150 ms debounce so dragging a control applies only its final value instead of
repeatedly resetting filter state or flooding the Bose bus.
`api.reboot_timeout: 0s` prevents loss of Home Assistant from rebooting an
otherwise-working audio renderer.

The Bose attenuation message remains the proven SmartSpeaker command
`02 00 AA CC`, where `CC` is the XOR checksum and higher `AA` is quieter. The
UI exposes the native attenuation number rather than presenting it as a
percentage.

Expected log after a control change and the next PCM chunk:

    PCM DSP ACTIVE: mono, LP 90.0 Hz LR4, low-cut OFF/0.0 Hz LR4, phase 0, crossover ACTIVE, 48000 Hz stream

Changing a crossover or phase setting resets filter state at a chunk boundary,
so a small click during active playback is possible while tuning.

Native tests cover live LP/HP changes, LR4 response, mono channel equality,
phase inversion, crossover bypass, clipping, chunk continuity, the PR #14389
ABI, and single application of upstream software volume. CI compiles the full
XIAO ESP32-S3 configuration, verifies all five HA entities and their restore
settings, confirms the newer source pins, rejects the legacy mDNS override, and
checks that the final ELF's PCM path calls the DSP wrapper.
