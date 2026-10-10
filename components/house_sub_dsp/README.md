# HOUSE subwoofer DSP — Home Assistant tuning controls

This branch keeps the accepted newer Snapclient PR #14389 baseline pinned at `luar123/esphome` commit `4d3280bd35fdd970e628a22197f19ab0fded1a39` and its Snapclient core `774268009d1a6c3d1664fad533409a22e5a41e83`.

The renderer remains mono `(L+R)/2` and now accepts runtime, no-recompile settings from ESPHome/Home Assistant:

- fourth-order Linkwitz-Riley low-pass;
- optional fourth-order Linkwitz-Riley low-cut/high-pass, with zero meaning off;
- 0/180-degree polarity inversion;
- crossover bypass while retaining mono summing and the selected phase.

The settings are handed from the ESPHome main loop to the decoder task through atomics. Filter coefficients and state are changed only in the audio task at a chunk boundary. No transport buffers, timestamps, queue lengths, I2S pins, Snapcast synchronization code or Bose control-bus code are replaced. Snapcast software volume still runs exactly once per fragment before the node DSP.

The companion YAML exposes Bose attenuation, low-pass, low-cut, phase and crossover bypass as native ESPHome entities. Restore values are applied on boot, while `api.reboot_timeout: 0s` prevents loss of Home Assistant from rebooting the audio renderer.

The Bose attenuation command remains the proven SmartSpeaker message `02 00 AA CC`, where `CC` is the XOR checksum and higher `AA` is quieter. The UI deliberately exposes the native 0–40 attenuation range rather than pretending it is a percentage.

Expected runtime log after a control change and the next PCM chunk:

    PCM DSP ACTIVE: mono, LP 90.0 Hz LR4, low-cut OFF/0.0 Hz LR4, phase 0, crossover ACTIVE, 48000 Hz stream

Native tests cover live LP/HP changes, LR4 response, mono channel equality, phase inversion, crossover bypass, clipping, chunk continuity, the PR #14389 ABI and single application of upstream software volume. The CI build also compiles the full XIAO S3 configuration with the Home Assistant entities and checks for the pinned newer baseline and absence of the legacy mDNS override.
