# HOUSE subwoofer DSP

Scope: mono `(L+R)/2`, then a fixed (YAML-time) 90 Hz fourth-order Linkwitz-Riley low-pass, duplicated into both PCM5102A output slots. No runtime controls, server/API changes, standby changes, or pin changes.

This add-on is specifically for `c-MM/esphome-snapclient` at `ce51e2fd861348698a6b4b7462fdda038cb942c9`, using its default stereo software-volume configuration and the HOUSE `48000:16:2` stream. Do not combine it with unrelated Snapclient implementations or arbitrary PCM formats. The upstream DSP API does not pass channel count or bit depth, so those remain explicit integration requirements, not runtime-detectable properties.

The upstream component depends on `CarlosDerSeher/snapclient` at `1adc5245012160c3c4eb312c962c7dc18b17231e`. Its decoder calls the C `dsp_processor_worker(char *, size_t, uint32_t)` entry point before inserting timestamped PCM chunks. We use GNU ld `--wrap` on that external call and call through to the original worker first, retaining existing software volume exactly once. Then we process in place, without changing chunk lengths, timestamps, transport buffers, or I2S configuration. `-fno-lto` is deliberate: cross-translation-unit inlining can otherwise bypass linker interposition.

Two Q=1/sqrt(2) Butterworth biquads implement the LR4 response. Filter state persists across chunk boundaries; it resets on sample-rate changes and long delivery gaps. PCM buffer access is 32-bit, matching the upstream allocator contract. Output saturates rather than wrapping on transient overshoot. No analog outputs should be tied together.

The new filter has frequency-dependent phase/group delay, and the Bose factory DSP remains downstream. Unchanged Snapcast timestamps do not guarantee unchanged acoustic phase alignment. Listening and hardware reliability still require field testing.

`PCM DSP ACTIVE: mono, 90.0 Hz LR4, 48000 Hz stream` is emitted from the actual processing hook, not just the startup/configuration path.

Native checks performed before delivery: left-only/right-only equality, identical output slots, in-phase sum scaling, opposite-polarity cancellation, LR4 response at 44.1/48 kHz, chunk-boundary continuity, saturation, silence, reset, rate changes, invalid arguments, linker call-through, and single application of upstream volume. Hardware playback is not yet field-proven for this DSP addition.
