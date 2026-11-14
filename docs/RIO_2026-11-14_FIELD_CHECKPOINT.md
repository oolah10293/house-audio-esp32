# Rio field checkpoint — 2026-11-14

## Scope and provenance

This is a **documentation-only checkpoint** on the existing source branch. It preserves the reported field observations from the November 14 conversation titled *Inspect Deployed Implementation* after the source-only commits `cb5cd61f2ac610ab032d28ce1056e12ac735fa01` and `4dab850eb7770c6b4b694d49c5380d454c80937a`.

The GitHub code diff was independently inspected; the device/house-playback observations below were **reported by the user** in that conversation and are not independent measurements or captured firmware/packet logs.

## Source changes confirmed on branch

- `firmware/rio-subwoofer.yaml` removes `esp_base_mac_addr_set` / the spoofed `44:BD:8D:EC:51:80` identity and uses the S3 native MAC.
- Snapclient dependency pinned to `luar123/esphome@4d3280bd35fdd970e628a22197f19ab0fded1a39`, the same revision as the Bose sub configuration.
- Rio's `house_sub_dsp` and `house_rio` components pinned to `oolah10293/house-audio-esp32@18503e36b866a6c9eed580359132503b3725cdb2`.
- Rio software audio-follow, 2-second Pi polling, DSP/crossover controls, and I2S pins were preserved.
- These changes were **not merged into `main`** when this checkpoint was written.

## Subsequent field observations (user report)

- After the source-only commit, the Rio configuration was installed/flashed onto the replacement S3, which then used its native identity.
- The old Rio Pi-side registration associated with the spoofed identity was deleted; the user reported that the deletion worked. The Rio using its real identity was registered and connected.
- The user then played from the CD-player S3 to **six listening nodes, including Rio**, and reported **no dropouts over two songs**.

The branch's earlier `components/house_rio/README.md` says "not flashed/not hardware tested" because it documents the **source-commit moment**, before these later field observations. This checkpoint records the later outcome, without claiming that a matching binary SHA, a build-log pass, or a long-duration stability test was independently verified.

## Remaining uncertainty

- Initial multi-node playback passed as reported; prolonged stability is not yet established by this checkpoint.
- The reason earlier Wi-Fi channel utilization/dropouts occurred remains **unproven**. Removing MAC spoofing and pinning dependencies together preceded the successful field result, but neither change has been isolated as the cause.
- No build, firmware package, installer, release, deploy or flash was performed by this documentation checkpoint.
