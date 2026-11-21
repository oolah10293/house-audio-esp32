# Final five-node ESPHome audio configuration — 2026-11-20

**Status: FINALIZED SOURCE BASELINE — deployment and field validation pending.** The canonical files are on `main` in `firmware/nodes/`. This document captures the agreed source configuration as of the last source edit, commit `ba6b3ee9008eaace532ebc1567017e75964bceb3`. The summary itself is documentation; it does not establish which firmware images are installed on hardware.

## Five-device inventory and IP assignments

| Device | YAML path under `firmware/nodes/` | Board | Static IP | I2S GPIOs (LRCLK / BCLK / DOUT) |
| --- | --- | --- | --- | --- |
| Motorola Radio | `house-audio-c6-3.yaml` | XIAO ESP32-C6 | `192.168.1.161` | `21 / 22 / 23` |
| Emerson Radio | `house-audio-c6-4.yaml` | XIAO ESP32-C6 | `192.168.1.179` | `21 / 22 / 23` |
| Basement stereo | `house-audio-s3-1.yaml` | XIAO ESP32-S3 | `192.168.1.117` | `4 / 5 / 6` |
| Music Room stereo | `house-audio-s3-2.yaml` | XIAO ESP32-S3 | `192.168.1.100` | `4 / 5 / 6` |
| Garage stereo | `house-audio-s3-5.yaml` | XIAO ESP32-S3 | `192.168.1.124` | `4 / 5 / 6` |

All five use network gateway `192.168.1.1`, subnet `255.255.255.0`, and Snapserver at `192.168.1.12:1704`. Addresses are set with `wifi.manual_ip` in each YAML.

**Pi-hole reservation status (owner confirmation):** The owner supplied screenshots showing the Pi-hole reservation entries and subsequently confirmed saving them. This is a **user-confirmed configuration**, not an independently queried live Pi-hole state. Recorded S3 MAC/address associations:

- Basement: `e0:72:a1:f8:4c:a8` → `192.168.1.117`
- Music Room: `e0:72:a1:f8:4c:98` → `192.168.1.100`
- Garage stereo: `44:bd:8d:ec:51:e0` → `192.168.1.124`

Motorola and Emerson were already shown with reservations when the additional three were configured. The owner confirmed the new entries were saved. Do not interpret this historical confirmation as continuous monitoring of Pi-hole.

## Shared source settings

All five YAMLs now specify:

```yaml
logger:
  level: WARN
  baud_rate: 0

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  fast_connect: true
  power_save_mode: none
  post_connect_roaming: false
  manual_ip:
    static_ip: <unique node IP from table>
    gateway: 192.168.1.1
    subnet: 255.255.255.0

ota:
  - platform: esphome

api:
  reboot_timeout: 0s

network:
  enable_ipv6: false
```

The `logger.baud_rate: 0` setting disables serial logger transport, retaining `WARN` messages via the ESPHome native API over Wi-Fi. All five use ESP-IDF with `framework.version: recommended` and `esphome.min_version: 2026.3.0`. All external Snapclient sources are pinned to commit hashes with `refresh: 0s`. The explicit `media_player.snapclient.hostname` and port are identical; their media-player names remain unique.

`fast_connect` does not guarantee association with the strongest AP on a multi-AP network; no measured AP-selection or startup-performance result for this updated baseline has been recorded. `power_save_mode: none` is explicitly configured for all five.

## Intentional processor-dependent differences — DO NOT normalize away

**ESP32-C6 radios (Motorola and Emerson):**

- CPU 160 MHz, flash 4 MB; I2S GPIO21/22/23; no `psram:` section.
- Snapclient external component source pinned to `github://oolah10293/house-audio-esp32@19e4679b1b8d96c5416f94836d54300e6f0a25d8`.
- That component delegates the core Snapclient components to `luar123/snapclient` commit `774268009d1a6c3d1664fad533409a22e5a41e83`, but **overrides only `dsp_processor`** with the patched component at `oolah10293/house-audio-esp32` commit `d135a2735cb59378f732bc1373607633491f1aaf`.
- **Preserve the C6 Q16.16 fixed-point stereo software-volume path.** The code uses integer gain per sample, rather than the upstream per-sample floating-point calculation, and bypasses scaling at unity gain. Its source comment identifies the C6's lack of an FPU and previously observed underruns as the reason. The owner explicitly reconfirmed the decision to retain this optimization. Do not silently replace the C6 dependency with the S3 source, or substitute upstream DSP, in a future standardization pass.

**ESP32-S3 stereos (Basement, Music Room and Garage):**

- CPU 240 MHz, flash 8 MB, `psram.mode: octal` and `psram.speed: 80MHz`; I2S GPIO4/5/6.
- All three now use the identical external-component pin: `github://oolah10293/esphome@7d84414a54235a215fd2420bd6a8ac00156d488e`.
- Their Snapclient integration uses the same underlying `luar123/snapclient` commit `774268009d1a6c3d1664fad533409a22e5a41e83` for all its components, **without the C6 DSP override**. This ESPHome fork contains a logging-tag compatibility correction; it is not a separate codec or streaming protocol.
- The owner confirmed the C6-specific DSP difference should remain and does **not** authorize changing S3 DSP just for uniformity.

All I2S pin mappings, device IDs and friendly names are intentionally retained.

## Verification and release boundary

**Source verified:** GitHub `main` contains these five YAMLs, the shared Wi-Fi/logging/API settings, pinned dependencies, and distinct C6/S3 I2S assignments. The eight preceding cleanup passes are recorded in [ESPHOME_AUDIO_NODES_CLEANUP_2026-11-20.md](ESPHOME_AUDIO_NODES_CLEANUP_2026-11-20.md). This page consolidates the final intended state; historical entries in that changelog refer to earlier states.

**User-confirmed:** Pi-hole reservations for the five audio-node addresses were reported completed, including the three new S3 reservations.

**User-reported configuration validation (2026-11-20):** all five canonical YAMLs passed `python -m esphome config` in the owner's existing Windows ESPHome 2026.4.0 environment, staged at `C:\esphome\staging`; see [the handoff checkpoint](ESPHOME_LOCAL_SETUP_AND_UPGRADE_HANDOFF.md#all-five-staged-yaml-validation--2026-11-20). The owner's five PASS lines were supplied, but full validation logs were not independently inspected. **Still unverified:** compiled firmware/toolchain success, installed firmware revision on each device, OTA/boot, native API logging, Wi-Fi association and audio performance with this final set of YAMLs. No performance/resource benchmark exists for the final configurations. Do not describe compilation or deployment as tested.

**Scope:** These five ESPHome audio renderers only. Do not include the distinct `garage-node.yaml` garage-door controller, the Bose or Rio subwoofer nodes, or the Lite-On CD device in this five-node sign-off. Their own state and documentation remain separate.

**Future changes:** Preserve the existing working local ESPHome setup and its rollback files. Inspect the local environment before upgrading; see [ESPHOME_LOCAL_SETUP_AND_UPGRADE_HANDOFF.md](ESPHOME_LOCAL_SETUP_AND_UPGRADE_HANDOFF.md). Firmware building, packaging and release require the owner's separate explicit authorization. Any later firmware changes should start from `main` and add a new checkpoint rather than rewriting this baseline's historical evidence.
