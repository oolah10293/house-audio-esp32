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

## Pass 3 — pin S3 Snapclient logging compatibility — 2026-11-20

User requested resolution of a known Snapclient source-incompatibility before future installations. Verified both reference sources against their GitHub histories:

- `luar123/esphome@snapclient2` resolves to commit `5a2bf5ab44658e984b2ede8ac4f1697b036740e9`, with `ESPHOME_LOG_TAG(TAG, "snapclient")` in the Snapclient header. This macro was incompatible with the user's older ESPHome toolchain during prior Garage compilation.
- The user's `oolah10293/esphome@fix-snapclient-log-tag` resolves to commit `7d84414a54235a215fd2420bd6a8ac00156d488e`. GitHub commit comparison from `5a2bf5ab...` to `7d84414a54235a215fd2420bd6a8ac00156d488e` proves there are **exactly four descendant commits and only two changed files**, both Snapclient logging-related: `esphome_snapclient.cpp` and `esphome_snapclient.h`. The incompatible header macro was removed and a traditional `static const char *const TAG` defined in the implementation. No Snapclient audio/transport processing sources differ in that comparison.
- Basement (`house-audio-s3-1.yaml`) and Music Room (`house-audio-s3-2.yaml`): replaced moving `luar123/esphome@snapclient2` reference with immutable `github://oolah10293/esphome@7d84414a54235a215fd2420bd6a8ac00156d488e`. Existing component selection and refresh settings remain unchanged.
- Garage (`house-audio-s3-5.yaml`): changed only `ref: fix-snapclient-log-tag` to `ref: 7d84414a54235a215fd2420bd6a8ac00156d488e`, preserving its existing `type: git` URL and `refresh: 1d`. This pins the exact source Garage already selected at review time, rather than silently following future branch updates.
- Motorola and Emerson C6 renderers remain pinned to their existing independent known-working component at `oolah10293/house-audio-esp32@19e4679b1b8d96c5416f94836d54300e6f0a25d8`. No other settings modified, including recently added `fast_connect`.

**Verified:** Git ancestry, patch scope (two logging files only), dependency references, and repo source-only diffs. **Not verified:** compatibility/build with the owner's currently installed ESPHome executable, ability to install compiled firmware, or behavior on the physical S3 hardware. No ESPHome config validation, firmware compilation, firmware package, installation or flash performed. Existing installed firmware remains untouched. Pinning an external-components commit does not guarantee it will compile with a given toolchain; do not represent this as field-tested before compiling under separately authorized build work.

## Pass 4 — standardized WARN logging over Wi-Fi — 2026-11-20

After explicit user instruction to finish logging standardization, updated the five GitHub YAML source files:

- All five: `logger.level: WARN` (replaces `NONE` on Motorola/Emerson, `DEBUG` on Basement/Music Room/Garage stereo).
- Basement stereo: add `api:\n  reboot_timeout: 0s` matching the existing four audio renderers. The ESPHome native API permits a connected log viewer to retrieve WARN/ERROR logs over Wi-Fi.
- Basement stereo: change `esp32.framework.log_level: INFO` to `ERROR`, matching ESPHome's documented default ESP-IDF framework logging level used by the other four nodes.
- All three S3 stereos: preserve `logger.hardware_uart: USB_SERIAL_JTAG`, leaving USB diagnostics available as before. C6 default logging transport settings remain unchanged.
- No changes to Wi-Fi addresses/fast_connect, OTA, Snapclient component sources, chip/PSRAM/I2S settings, names or IDs.

**Verification performed:** source-text checks for one WARN logger on each file, all five native API stanzas, GitHub commit diff review. **Not performed:** local ESPHome configuration parsing, compilation, installed firmware or runtime performance testing. Native API network log capture and logger semantics verified against the published ESPHome logger/API documentation; actual connected logging performance was not hardware-tested.

## Pass 5 — static IP addresses for three S3 stereo nodes — 2026-11-20

The owner supplied a screenshot of network client/device records associating the following **hostnames, IP addresses, and MAC addresses**:

| Device | ESPHome hostname | Configured static IP | Reported MAC |
| --- | --- | --- | --- |
| Basement stereo | `house-audio-s3-1` | `192.168.1.117` | `e0:72:a1:f8:4c:a8` |
| Music Room stereo | `house-audio-s3-2` | `192.168.1.100` | `e0:72:a1:f8:4c:98` |
| Garage stereo | `house-audio-s3-5` | `192.168.1.124` | `44:bd:8d:ec:51:e0` |

Added to each S3's `wifi:` section:

```yaml
manual_ip:
  static_ip: <device address from screenshot>
  gateway: 192.168.1.1
  subnet: 255.255.255.0
```

Gateway and subnet match the existing Motorola/Emerson C6 node configurations in this repository. The Music Room's existing `wifi.use_address: 192.168.1.100` is intentionally retained, since it matches its static IP and may be used for management and upload targeting. All other YAML values were retained.

**DHCP conflict precaution:** The supplied screenshot demonstrates IP/hostname/MAC associations but does **not by itself prove** they are fixed DHCP reservations or excluded from the DHCP allocation pool. Before using static addresses on the network, check Pi-hole DHCP reservations for these MAC/IP pairs (or remove those IPs from the dynamic DHCP pool) so another device cannot receive them. Do not mistake an observed active lease for a confirmed reservation.

**Verified:** source changes to three YAMLs and GitHub diff. **Not verified:** Pi-hole reservation state, current DHCP allocation scope, ESPHome configuration validation, or physical network behavior. This is a source-only checkpoint.

## Pass 6 — remove redundant S3 configuration differences — 2026-11-20

At the owner's explicit request, made these **three exact source cleanups**:

1. Basement stereo: removed only `esp32.framework.log_level: ERROR`; the YAML now uses the same unspecified framework log level as the other S3 configurations (the previously verified ESPHome default is ERROR). Its `logger.level: WARN` is unchanged.
2. Music Room stereo: removed only `wifi.use_address: 192.168.1.100`; its actual `wifi.manual_ip.static_ip: 192.168.1.100` remains, with existing network parameters intact.
3. Garage stereo: rewrote `external_components` to the identical short-form `github://oolah10293/esphome@7d84414a54235a215fd2420bd6a8ac00156d488e` and `refresh: 0s` used by Basement and Music Room. The **exact immutable Snapclient source commit** is unchanged; only the declaration syntax and refresh policy changed.

**Source verification:** inspected original GitHub YAML at prior `main`, applied only those three replacements, and reviewed the resulting GitHub patch. No other component setting or node identity changed. Build/runtime compatibility has not been separately verified.

## Pass 7 — disable S3 serial logging, preserve Wi-Fi logs — 2026-11-20

The owner reported inaccessible USB ports on two of three S3 audio renderers and requested eliminating unnecessary JTAG logging. ESPHome's published logger documentation (https://esphome.io/components/logger/) and the ESPHome 2026.4.0 logger source show that the S3 defaults to USB_SERIAL_JTAG for console logging; simply deleting the explicit `hardware_uart` setting would re-enable the same default. Set `logger.baud_rate: 0` instead, retaining `logger.level: WARN` and the already standardized `api.reboot_timeout: 0s`.

Three S3 YAMLs received the same exact replacement:

```yaml
logger:
  level: WARN
  baud_rate: 0
```

Targets: Basement `house-audio-s3-1`, Music Room `house-audio-s3-2`, Garage stereo `house-audio-s3-5`.

Verification: confirmed `baud_rate: 0` is documented by ESPHome as disabling serial logging while ESPHome's native API still provides remote logger messages; verified exactly three source-file logger edits in the GitHub diff. Runtime remote logging has not been exercised with these YAMLs. This affects the logger's serial output, not the board's USB flashing interface or the actual Snapclient transport.

## Pass 8 — align C6 logger transport with S3 nodes — 2026-11-20

At the user's request, added `logger.baud_rate: 0` to **Motorola** (`house-audio-c6-3`) and **Emerson** (`house-audio-c6-4`), keeping their existing `logger.level: WARN`. This matches the three S3 renderers' logger configuration exactly:

```yaml
logger:
  level: WARN
  baud_rate: 0
```

All five continue to have `api.reboot_timeout: 0s` for remote log viewing through the ESPHome native API. No Wi-Fi, Snapclient, I2S, device-identity, or address configurations changed. Only two C6 YAMLs and this checkpoint document changed.

**Verified:** GitHub source edit and commit diff for the exact addition on each C6 node, and inspection of existing five-node logger/API declarations. **Not separately verified:** firmware compilation or hardware performance/resource effect.
