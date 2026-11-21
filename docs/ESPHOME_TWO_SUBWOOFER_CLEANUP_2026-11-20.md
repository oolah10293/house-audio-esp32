# Rio and Bose subwoofer ESPHome cleanup — source checkpoint (2026-11-20)

## Authority and scope

User supplied two current local YAML sources: `rio-subwoofer(1).yaml` and `bose-subwoofer.yaml`. These are **two separate ESP32-S3 subwoofer renderers** outside the five-node House Audio standard audio rollout. The newer Bose upload corresponds to the `bose-sub-auto-standby` branch's `firmware/bose-subwoofer-prototype.yaml`, not the older Bose prototype on `main`. Preserve that older prototype as history.

For the current source cleanup:
- Canonical Rio source: `firmware/rio-subwoofer.yaml` on `main`.
- New canonical Bose source: `firmware/bose-subwoofer.yaml` on `main` (the old `firmware/bose-subwoofer-prototype.yaml` remains unchanged).
- Both configurations retain their own exact immutable Snapclient and DSP/control component revisions; **do not force the five standard renderers' dependencies onto them**.
- Both still require ESPHome >=2026.4.0 and native ESP-IDF on XIAO ESP32-S3.

## Exact source-only edits

Both subwoofers:
1. Change `logger.level: DEBUG` to `WARN`.
2. Change `logger.hardware_uart: USB_SERIAL_JTAG` to `baud_rate: 0` (disable serial logging; leave native API logs accessible).
3. Add `wifi.fast_connect: true`; preserve `post_connect_roaming: false`, `power_save_mode: none`, secrets, and all existing control features.

Bose only: remove explicit `esp32.framework.log_level: INFO`, matching the streamlined S3 configurations.

**Intentionally unchanged:**
- Rio: native MAC/Snapcast identity (never reintroduce spoofed MAC); Pi registration, `house_rio` follow policy, `house_sub_dsp` audio muting and lowpass/lowcut/phase controls; snapclient pin `4d3280bd35fdd970e628a22197f19ab0fded1a39` and Rio component pin `18503e36b866a6c9eed580359132503b3725cdb2`.
- Bose: `house_bose` acknowledged standby/wake, GPIO43/44 open-drain/UART pins, hardware attenuation and Pi control API, startup restore and crossover controls; snapclient pin `4d3280bd35fdd970e628a22197f19ab0fded1a39` and Bose control pin `92608cf75bf470e91de00fb08437a606f7adeb96`.
- Neither source assigns a static IP as of this checkpoint. **Do not guess a DHCP reservation.** Historically observed device addresses from user reports were Bose `192.168.1.110` and Rio `192.168.1.129`, but the current leases and Pi-hole reservations were not independently established. Confirm MAC/address reservations before adding `wifi.manual_ip`. The House server `192.168.1.12` is **not** a renderer IP.

## Verification and next step

- **Verified:** The current source revisions and code differences were inspected, and the modified YAML structures parsed with a generic YAML parser. These checks do **not** constitute ESPHome `config` or `compile`.
- **Not yet done:** ESPHome configuration validation on the user's local environment, compilation, flash, or device audio/standby/follow regression testing of these two updated YAMLs. Previously working user-installed subwoofer firmware is not evidence that these new revisions have been deployed.
- **Deployment sequence:** (1) confirm Bose and Rio current Pi-hole MAC-to-IP reservations; (2) if desired, add the confirmed static IPs to both source configurations; (3) stage both YAMLs using the already working `C:\esphome\staging` directory and its local `secrets.yaml` (never commit secrets); (4) `esphome config` each; (5) compile both **once**, then upload existing binaries separately; (6) field-check DSP/sub follow and Bose standby/wake as well as audio and synchronization.
- Preserve the working `C:\esphome\configs`, the timestamped backup `C:\esphome\configs-backup-20261120-202826`, and the installed ESPHome 2026.4.0 toolchain. No upgrade is needed for this cleanup.

A request for source edits is not a request for the assistant to build, package, release or flash. When explaining commands the owner can run on their own PC, do not repeatedly demand authorization for operations the assistant cannot perform.
