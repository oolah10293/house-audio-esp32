# Audio-node YAML baseline and cleanup — 2026-11-20

## Scope and authority

This is a **source-only** checkpoint using the owner's five uploaded ESPHome audio-node YAMLs. The separate garage-door/sensor controller `garage-node.yaml` was explicitly excluded. The uploaded files are the source for this checkpoint; they are **not independently verified as the firmware currently flashed** to hardware. Keep originals on the PC as the rollback source.

## Pass 1 — filenames only, zero configuration changes

Five YAML files were copied to `firmware/nodes/`. File contents are **byte-for-byte identical** to the corresponding uploads; this checkpoint modifies filename/path only, not any YAML key or value.

| Uploaded file | Canonical repository file | Existing device ID | Device | Hardware |
| --- | --- | --- | --- | --- |
| `house-audio-s3-3.yaml` | `house-audio-c6-3.yaml` | `house-audio-c6-3` | Motorola Radio | XIAO ESP32-C6 |
| `house-audio-c6-4.yaml` | `house-audio-c6-4.yaml` | `house-audio-c6-4` | Emerson Radio | XIAO ESP32-C6 |
| `house-audio-s3-1.yaml` | `house-audio-s3-1.yaml` | `house-audio-s3-1` | Basement stereo | XIAO ESP32-S3 |
| `house-audio-s3-2.yaml` | `house-audio-s3-2.yaml` | `house-audio-s3-2` | Music room stereo | XIAO ESP32-S3 |
| `house-audio-c6-5.yaml` | `house-audio-s3-5.yaml` | `house-audio-s3-5` | Garage stereo | XIAO ESP32-S3 |

**Important:** no `esphome.name` or `friendly_name` was changed; preserving these avoids unnecessary changes to discovered device identity.

## Observed differences intentionally not altered

- Snapclient source: Motorola/Emerson reference immutable `oolah10293/house-audio-esp32@19e4679b1b8d96c5416f94836d54300e6f0a25d8`; Basement/Music Room reference `luar123/esphome@snapclient2`; Garage stereo references `oolah10293/esphome@fix-snapclient-log-tag` (`refresh: 1d`). Do **not** treat these as interchangeable without further investigation.
- Static address: Motorola `192.168.1.161`; Emerson `192.168.1.179`. Music Room's `wifi.use_address: 192.168.1.100` is a connection/upload target **not** a static IP configuration. No static IP supplied in the other S3 YAMLs.
- Logger: C6 nodes `NONE`, S3 nodes `DEBUG`. Basement ESP-IDF separately includes `log_level: INFO`. Basement YAML lacks an `api:` block present in other audio YAMLs.
- C6 pins GPIO21/22/23 vs S3 pins GPIO4/5/6; these must remain hardware-specific.
- Power saving disabled and IPv6 disabled in all five uploads. Secrets are referenced through `!secret`; no credentials were uploaded or committed.

## Checks and rollout status

The five uploaded YAMLs were read directly; filenames were selected by existing `esphome.name` and ESP32 board type, **not** by the misleading filenames. All five candidate copies matched the original files byte-for-byte, and YAML structures were compared unchanged. This is **source-only; no ESPHome config check, compile, package, OTA, flash, device reboot, or audio regression test was performed**.

Next, perform a read-only comparison of Snapclient implementations and document which settings are deliberate hardware differences and which should be harmonized. Do not alter known-working dependency revisions or introduce shared YAML package dependencies as part of cosmetic standardization. Any firmware build/packaging/installation requires explicit authorization.

## Pass 2 — Wi-Fi fast connect on three S3 renderers — 2026-11-20

User explicitly requested applying the Motorola startup-speed optimizations to other nodes that lack them. Compared the five user-uploaded YAML configurations. Motorola C6-3 and Emerson C6-4 already had both `fast_connect: true` and `wifi.manual_ip`; three S3 stereo renderers had neither. The S3 source files now contain exactly one new line each, `fast_connect: true` immediately after `wifi.password`:

- `house-audio-s3-1.yaml` — Basement stereo
- `house-audio-s3-2.yaml` — Music room stereo
- `house-audio-s3-5.yaml` — Garage stereo

**Not yet applied: manual/static IP.** Actual stable, nonconflicting S3 addresses and DHCP reservations are not verified. In particular, Music Room's `use_address: 192.168.1.100` specifies the device address to target for management; it does **not** by itself configure static IP. Do not guess or assign addresses without checking the current network. `fast_connect` avoids initial Wi-Fi scanning, but ESPHome warns that it may attach to the first AP it sees instead of the strongest; the home has multiple APs. Measure real power-on-to-audio join time and AP association before attributing improvements.

No other YAML values (including device names, MACs, pins, `use_address`, Snapclient source, logging, power save, OTA, API) changed. **Source edits and diff inspection only**; ESPHome config validation, firmware compilation, installation, and field performance remain untested. Never build/package/flash without explicit authorization.
