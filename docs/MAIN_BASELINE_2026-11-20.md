# Main baseline consolidation — 2026-11-20

User requested correction of branch fragmentation. Source-only; no build/package/flash/deployment authorized.

- CD history: 39d1e7ce6eede9f490a0a23df6712ced57b0d46c; hardware-confirmed 3.4 source f7741b4c9faf160e84c8ca9fb5dde6da7989d59c. Every tracked file in HouseAudioCDInput matches the hardware-confirmed source by Git blob SHA. See cd-arduino/docs/CD_PLAYER_3_4_INSTALL.md.
- Rio history: fe84676e92665f96b468d7a4dca80545aee7c825; native MAC and pinned Snapclient, user reported working installation and two-song six-node playback. See RIO_2026-11-14_FIELD_CHECKPOINT.md. Common components and Bose prototype YAML from this lineage replace the older prototype on main. This does not independently establish the exact current Bose firmware. All immutable dependency pins preserved.
- Original main d8fc22712a5100d979557afc5aa9db5dca8f1d4e preserved as first parent. Older CD, Rio and Bose predecessors retained for history. The separate Bose auto-standby branch and C6 fixed-point candidate are not established as deployed baselines; do not blindly replace the working source with those alternatives. Recovery branch and release tag remain rollback provenance.
- New source work starts from main. Explicit authorization required for compilation/package/release; main CD workflow manual only. Historical publication workflows are not triggered on main. Prior 3.4 package metadata is historical: update version and delivery gate before any future authorized build.

## Network observations from user screenshots/reports
- Basement channel 6: ~10% paused, ~20% library playback (unchanged with idle Lite-On), up to 47% CD playback. Driveway 1, Guest Room 11. Moving the CD player to Guest Room added ~20 percentage points there.
- Lite-On: Basement -44 dBm/26 Mbps uplink; Guest Room -36 dBm/39 Mbps uplink; 72.2 Mbps downlink. Earlier observed CD upload 2.58 Mbps.
- Basement 09:30: 126k Tx packets, 143k Rx packets, 27.6k Tx retries, 9,924 Rx retries, zero dropped packets. Retry counts are not packet-loss rates. Guest Room also spiked during reported dropouts.
- Current CD code emits 44.1 kHz stereo 16-bit PCM WAV, eight sectors per read with no explicit pacing; Wi-Fi power save not explicitly disabled in Arduino 2.0.14. These are source observations, not established causes. Investigate power saving and lossless FLAC separately; no behavior changes in this checkpoint.
- No new hardware acceptance, builds, packages or installations.
