# ESPHome local setup and upgrade handoff

Last reviewed: 2026-11-20. This is a **documentation-only** handoff. It does not authorize updates, builds, firmware uploads, merges, or replacement of a working installation.

## Why this note exists

The owner runs ESPHome for several ESP32-based house-audio renderers and other devices. The local development setup was difficult to get working originally. Do not ask the owner to reconstruct installation history or GitHub branch history from memory when it can be recovered from the repositories and previous conversations.

**Practical priority:** Preserve the working Windows-based ESPHome installation, its launcher, and existing YAML configurations. Investigate an update in an isolated environment; do not make an in-place upgrade the first experiment.

## Assistant action boundary — repeated authorization mistake (2026-11-20)

The owner explicitly corrected the assistant **three times** for repeatedly appending `no flashing without your go-ahead` or asking for approval for actions that the assistant **cannot perform on the owner's Windows PC**. This instruction is a persistent workflow correction, not an authorization to execute a future operation.

- **When the assistant cannot execute the operation:** Provide the exact, ready-to-run command or plan, distinguish what it does from what it does not do, and let the owner decide whether to execute it on their own machine. **Do not frame this as a request for the assistant's permission to compile or flash, and do not add repetitive authorization disclaimers to every response.**
- **When the assistant can actually perform an action:** Require the owner's explicit authorization before the assistant initiates a firmware/software build, packaging, installer, release, or flashing operation. Discussing code, suggesting a command, agreeing on a design or replying "sounds good" does not grant the assistant that authorization. This safeguard is important; apply it to actions the assistant can really take.
- **Documentation, read-only inspection and progress checkpoints:** Can proceed as part of authorized ongoing assistance. Accurately label verified versus unverified results. Never imply an update happened on a device without the owner's reported evidence.
- **Windows execution environment:** Do not invoke plain `python -m esphome` in command examples unless its interpreter is independently verified. The owner's global `python` resolved to `C:\Users\John\AppData\Local\Programs\Python\Python314\python.exe` and lacked ESPHome. The previously observed ESPHome interpreter is `C:\Users\John\AppData\Roaming\uv\tools\esphome\Scripts\python.exe`; verify its existence and version before compilation.

**Compiler checkpoint from owner-supplied logs:** A five-file batch command that invoked the global Python 3.14 failed on **all five** with `No module named esphome`. These failures occurred **before firmware compilation**; no firmware compile status is established from that attempt. A corrected batch command using the explicit `uv\tools\esphome\Scripts\python.exe` interpreter was then supplied. **Owner-reported results (2026-11-20): five of five compile commands returned PASS:** `house-audio-c6-3.yaml`, `house-audio-c6-4.yaml`, `house-audio-s3-1.yaml`, `house-audio-s3-2.yaml`, and `house-audio-s3-5.yaml`. These are the exact user-pasted PowerShell results; complete compilation logs and output firmware paths/hashes have not yet been inspected. **Owner-reported firmware artifact check (2026-11-20):** After the five compile PASS reports, the owner ran a read-only PowerShell search and supplied the five concrete `firmware.bin` paths and sizes:

| Node | Size (bytes) | Path relative to `C:\esphome\staging\.esphome\build\` |
|---|---:|---|
| house-audio-c6-3 | 986448 | `house-audio-c6-3\.pioenvs\house-audio-c6-3\firmware.bin` |
| house-audio-c6-4 | 986464 | `house-audio-c6-4\.pioenvs\house-audio-c6-4\firmware.bin` |
| house-audio-s3-1 | 879312 | `house-audio-s3-1\.pioenvs\house-audio-s3-1\firmware.bin` |
| house-audio-s3-2 | 879328 | `house-audio-s3-2\.pioenvs\house-audio-s3-2\firmware.bin` |
| house-audio-s3-5 | 879312 | `house-audio-s3-5\.pioenvs\house-audio-s3-5\firmware.bin` |

Evidence level: this verifies the existence and reported sizes **from the owner's local command output**, not through direct access to the owner's Windows filesystem. Contents, hashes, boot and OTA behavior remain unverified. **Next checkpoint:** review the difference between the owner's live `C:\esphome\configs\house-audio-s3-1.yaml` and the staged `C:\esphome\staging\house-audio-s3-1.yaml`, then plan the Basement stereo's OTA using its existing binary; do not recompile all five. Do not repeat compilation unless evidence shows a build artifact is missing or unusable.

## Basement stereo live-versus-staged comparison — owner report, 2026-11-20

The owner ran `fc.exe /N "C:\esphome\configs\house-audio-s3-1.yaml" "C:\esphome\staging\house-audio-s3-1.yaml"` and posted its difference hunks. Compared against the current `firmware/nodes/house-audio-s3-1.yaml` in GitHub `main`, all differences are expected and intentional:

- Remove explicit framework `log_level: INFO` (rely on documented default).
- Change logger `DEBUG` / `hardware_uart: USB_SERIAL_JTAG` to `WARN` / `baud_rate: 0`.
- Add `wifi.fast_connect: true`.
- Add static IP `192.168.1.117`, gateway `192.168.1.1`, subnet `255.255.255.0`.
- Add `api.reboot_timeout: 0s`.
- Replace moving `github://luar123/esphome@snapclient2` with pinned logging-compatibility fork `github://oolah10293/esphome@7d84414a54235a215fd2420bd6a8ac00156d488e`.

**Evidence:** Owner-pasted `fc.exe` output includes only these six groups of changes. The staged file passed ESPHome config and compile in the owner's prior checks. A built Basement `firmware.bin` was found at `C:\esphome\staging\.esphome\build\house-audio-s3-1\.pioenvs\house-audio-s3-1\firmware.bin` (879312 bytes, owner-reported). Hardware boot, audio performance and OTA installation remain untested. The existing local configurations were not overwritten by this comparison.

**Next field action:** Controlled Basement stereo OTA to `192.168.1.117` with the **precompiled** firmware and staged YAML, then check Snapcast reconnect, audible output, synchronization, volume control and Wi-Fi stability before moving on to the other four. The CLI documented form `esphome upload <staged.yaml> --device 192.168.1.117 --file <existing firmware.bin>` uploads the prebuilt image without invoking compilation. An OTA upload is a distinct owner-executed action; do not state it has happened until the owner reports the result.

## Basement stereo OTA pilot — owner-reported field result, 2026-11-20

The owner reported: **"it flashed and started playing again"** after executing the previously supplied precompiled-bin OTA command for Basement stereo (`house-audio-s3-1`, `192.168.1.117`). This is **user-confirmed evidence** of completed OTA, restart/reconnection and audible playback. The chat did not include full upload logs or independent network telemetry; do not claim separate verification from the device.

**Accepted so far:** OTA command completed per owner's report, node returned to audible playback. **Not yet field checked:** sustained audio stability, synchronization with a second node, volume control, API logger connection, the exact flashed firmware hash. These must not be silently marked PASS.

The other four node binaries exist in the owner's staging directory per their path/size checks, but none has been reported installed in this rollout. The sensible next action is a short listening/synchronization/volume check on Basement, then sequential OTA uploads of the four **already compiled** binaries (`house-audio-c6-3` at `.161`, `house-audio-c6-4` at `.179`, `house-audio-s3-2` at `.100`, `house-audio-s3-5` at `.124`), capturing each outcome. Do not mistake a suggested batch command for a deployment that has actually happened. Preserve working configs and the existing backup.

## Basement stereo pilot acceptance — owner-reported, 2026-11-20

After confirming that the Basement stereo firmware OTA completed and audible playback resumed, the owner additionally reported: **"synchronized and snapcast volume control is working"**. Mark this pilot's three field checks **PASS by owner report**: audible playback, synchronization against another renderer, and Snapcast volume control. This does not establish long-term dropout-free operation or acceptance of the separate ESP32-C6 fixed-point DSP path.

Next rollout group: four remaining precompiled firmware images — Motorola radio `house-audio-c6-3` at `192.168.1.161`, Emerson radio `house-audio-c6-4` at `192.168.1.179`, Music Room stereo `house-audio-s3-2` at `192.168.1.100`, Garage stereo `house-audio-s3-5` at `192.168.1.124`. Preserve the current `C:\esphome\configs` directory and backup. A sequential PowerShell batch should use `uv\tools\esphome\Scripts\python.exe -m esphome upload` with the corresponding **already compiled** `firmware.bin` per device, capture separate logs, and stop at first upload failure. User confirmation of each device returning to audio remains pending.

## Remaining four OTA uploads — owner-reported PASS, 2026-11-20

The owner executed the previously supplied sequential PowerShell batch using existing precompiled binaries and posted these results:

```text
Uploading house-audio-c6-3 (192.168.1.161) ...
PASS: house-audio-c6-3
Uploading house-audio-c6-4 (192.168.1.179) ...
PASS: house-audio-c6-4
Uploading house-audio-s3-2 (192.168.1.100) ...
PASS: house-audio-s3-2
Uploading house-audio-s3-5 (192.168.1.124) ...
PASS: house-audio-s3-5
All four OTA uploads completed successfully.
```

**Evidence:** four ESPHome OTA command exit statuses as summarized by the owner's PowerShell output. No individual upload logs or independent post-boot network/device telemetry were provided. All five targeted nodes now have **reported successful OTA uploads**, including the previously accepted Basement stereo (`.117`). The Basement's audible playback, sync, and Snapcast volume response were subsequently confirmed by the owner. The other four have **not yet been field-confirmed** for audible playback, synchronization, volume or dropout-free stability in this chat.

**Next checkpoint:** ask the owner to play music through the four additional nodes and confirm reconnect/stream reception, synchronization, and Snapcast volume behavior. Do not classify an upload PASS as a separate hardware acceptance PASS. Keep the compiled artifacts and original live configuration backup until field acceptance is complete.

## Current Windows deployment observations — 2026-11-20

The user provided screenshots of the running Windows ESPHome dashboard and its console, and separately ran the read-only version command.

- **ESPHome version:** `2026.4.0` (user-supplied `Version: 2026.4.0` output).
- **Working configuration directory:** `C:\esphome\configs` (dashboard console explicitly displays `Config folder: C:\esphome\configs`).
- **Dashboard URL:** `http://localhost:6052`, seen open in the browser.
- **Running dashboard process:** console logs invoke `C:\Users\John\AppData\Roaming\uv\tools\esphome\Scripts\python.exe` to run `-m esphome` for commands. This verifies that executable was used by the dashboard, but **not** the complete launcher's contents or installation history.
- **Local filenames matter:** live dashboard lists Motorola as `house-audio-s3-3.yaml` and Garage stereo as `house-audio-c6-5.yaml`. Canonical GitHub copies are `firmware/nodes/house-audio-c6-3.yaml` and `firmware/nodes/house-audio-s3-5.yaml`, respectively, because their internal names and hardware types differ from those old filenames. Do not rename local files without checking deployment/OTA references.
- **ESPHome minimum version:** all five finalized YAMLs require `2026.3.0`, so `2026.4.0` satisfies the declared minimum. Component compatibility and actual compilation are not yet verified.

### Working configuration backup — destination and procedure

**Source (do not modify):** `C:\esphome\configs`

**Backup destination (timestamped sibling folder):**
`C:\esphome\configs-backup-YYYYMMDD-HHmmss`

For example, the following is an **example name only, not an asserted existing folder**:
`C:\esphome\configs-backup-20261120-202700`.

Run this from a separate PowerShell window to copy the entire configuration folder without replacing or moving the original:

```powershell
$backup = "C:\esphome\configs-backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
Copy-Item -LiteralPath "C:\esphome\configs" -Destination $backup -Recurse -Force
Write-Host "Backup created at: $backup"
Test-Path "$backup\secrets.yaml"
```

The exact resulting timestamped path must be copied from PowerShell output and **recorded after completion**, not guessed from the current date. Confirm the destination exists and contains the expected YAML files and `secrets.yaml`; a successful copy should not overwrite `C:\esphome\configs`. `secrets.yaml` must remain local and must **never** be committed to GitHub.

**Backup status (2026-11-20): USER-CONFIRMED COPY COMPLETED.** The user pasted the PowerShell transcript showing the `Copy-Item` command returning without an error, `Write-Host` reporting the exact destination **`C:\esphome\configs-backup-20261120-202826`**, and `Test-Path "$backup\secrets.yaml"` returning **`True`**. This confirms command completion and the presence of `secrets.yaml` as reported by the Windows machine; the entire directory tree and any firmware/rollback binaries were **not independently audited**. This backup was made by copying the live configuration directory and is not necessarily a backup of the dashboard launcher or toolchain. The actual backup files, including `secrets.yaml`, remain local; none were uploaded to GitHub. If a full rollback is later needed, do not automatically overwrite the live config folder; identify the precise backup and restore plan first.

**Next intended checkpoint:** Compare the locally running Basement stereo YAML with the finalized GitHub version before any pilot firmware update. Maintain this timestamped backup and preserve the working Windows launcher and installed ESPHome toolchain.

## All-five staged YAML validation — 2026-11-20

**User-reported result: PASS for all five**, using the existing `2026.4.0` Windows ESPHome Python environment and a staged directory `C:\esphome\staging`. The owner ran the previously supplied PowerShell loop downloading the five canonical GitHub YAMLs from the fixed `main` commit `98497355fbf6c03afec042ac95dca37630354b76` and copying `secrets.yaml` from the working config folder, then executing `python -m esphome config` on each one.

The owner pasted the following exact summarized results:

```text
PASS: house-audio-c6-3.yaml
PASS: house-audio-c6-4.yaml
PASS: house-audio-s3-1.yaml
PASS: house-audio-s3-2.yaml
PASS: house-audio-s3-5.yaml
```

**Evidence level:** This is direct user-provided PowerShell output of five zero-exit-status configuration checks; the individual `*.validation.log` files were not provided or inspected. The `esphome config` command checks configuration syntax and component setup; it is **not** an ESP-IDF toolchain build or a device/OTA test. No compile/test/deployment result is established. The live configurations in `C:\esphome\configs` were not intentionally replaced during staging. The timestamped pre-change backup remains `C:\esphome\configs-backup-20261120-202826`.

**Next step (owner's Windows PC):** The assistant should give the owner one command to compile all five staged configurations in a batch with individually captured logs, then help interpret the results and plan controlled OTA installation. The assistant cannot run commands on the owner's PC, so **do not ask the owner for authorization as if the assistant were about to run them**. Separate explicit owner authorization remains necessary before the assistant itself initiates a build, package, firmware release or installation when it has the capability to do so. Do not assume a successful `config` check establishes compilation success.

## User-reported local setup/history (not independently inspected)

- The working ESPHome installation is on a PC (not the Raspberry Pi). A launcher written during the project opens a command-line window; the owner then uses a browser interface. The launcher contents and exact command are **not yet inspected**.
- The installation method that finally worked was described in an earlier conversation as "some uv bullshit"; the previously identified command was `uv tool install esphome`. This is **historical recollection**, not verification of the PC's current installed packages or version.
- Before that, attempts to install/compile on a Raspberry Pi ran into insufficient memory. At that earlier point the Pi was reportedly used for Pi-hole; the house music server was created later. Attempts across three computers took roughly one to two weeks before the PC workflow was reliable.
- The owner is understandably reluctant to break an established, working setup. No update has been authorized merely by discussing one.

## Verified repository references (checked 2026-11-09)

1. [oolah10293/esphome](https://github.com/oolah10293/esphome) has branches `dev` and `fix-snapclient-log-tag`. The latter includes commits dated 2026-10-27 (UTC 2026-10-28) addressing Snapclient logging/tag compilation errors:
   - [Define Snapclient logging tag in implementation](https://github.com/oolah10293/esphome/commit/0699616a78cfef97069128631b506b97370a8697)
   - [Remove unsupported Snapclient logging macro from header](https://github.com/oolah10293/esphome/commit/e134fd75db76601c601b52c53ce3152b3eeaf00d)
   - [Use compatible static logging tag in Snapclient implementation](https://github.com/oolah10293/esphome/commit/7d84414a54235a215fd2420bd6a8ac00156d488e)
   The branch is preserved. **Do not assume this custom fix is still required for a current ESPHome version** without checking upstream.
2. [oolah10293/house-audio-esp32](https://github.com/oolah10293/house-audio-esp32) has branches `main`, `bose-sub-auto-standby`, `bose-sub-crossover`, `bose-sub-ha-controls`, `feature/rio-audio-follow`, and `fix/c6-fixed-point-volume`. Branch names identify areas of work but **do not establish which local YAML files use them**.
3. That repository's [README](https://github.com/oolah10293/house-audio-esp32/blob/main/README.md) documents the Snapclient tests, including an upstream ESPHome PR `github://pr#14389` used during testing. Its checked-in [Bose prototype YAML](https://github.com/oolah10293/house-audio-esp32/blob/main/firmware/bose-subwoofer-prototype.yaml) instead references `github://c-MM/esphome-snapclient@main`. These are documented examples, **not proof of what currently runs on the user's devices**.
4. There may be additional node YAML configurations stored locally on the PC and not checked into these repositories.

## Verified upstream changes to account for

- ESPHome 2026.6 introduced the new Device Builder in place of the legacy dashboard: https://esphome.io/blog/2026/06/17/esphome-2026-6/
- The old built-in `esphome dashboard` is no longer the supported graphical interface; Device Builder is a separate program. Official CLI documentation: https://esphome.io/guides/cli/
- The YAML-based CLI workflow remains supported (`esphome config`, `esphome compile`, `esphome run`): https://esphome.io/install/getting-started/
- The official installation instructions give `esphome-device-builder <configuration-directory>` for the GUI: https://esphome.io/install/
- ESPHome 2026.7 also changed ESP32 default toolchain selection and dropped Python 3.11 support; review release notes before choosing a target version: https://esphome.io/blog/2026/07/15/esphome-2026-7/

These upstream documentation references **do not mean the local installation was inspected or that the existing launcher is incompatible**.

## Minimal-risk plan for a future authorized update

1. On the working PC, **read only**: record the exact ESPHome version, `uv` installation/version if applicable, the launcher's full text, which directory it opens, and where the YAML files and `secrets.yaml` actually reside. Avoid publishing secrets to GitHub.
2. Make a local backup of the working launcher, YAML configuration tree (including secrets, handled privately), and enough installation details to reproduce it. Do not run an upgrade yet.
3. Inventory all actual `external_components:` references from **local live YAML**, including branches/PR references and refresh settings. Compare those dependencies with the repository branch history and current upstream components.
4. Set up the new ESPHome/Device Builder separately, while keeping the current launcher and its environment untouched. Determine the installation approach only after inspecting how the PC currently launches ESPHome.
5. Choose **one** representative device and validate/compile its YAML without flashing anything. Address any component or toolchain compatibility issue in isolation. Avoid broad code rewrites.
6. Only when test builds work and the owner expressly requests it, decide whether to migrate the day-to-day launcher or upload firmware. Preserve an easy fallback.

## Unresolved / must not guess

- Exact contents/path of the dashboard launcher and full `uv` installation details beyond the observed Python executable. The Windows ESPHome version (`2026.4.0`), live config path (`C:\esphome\configs`) and timestamped backup path (`C:\esphome\configs-backup-20261120-202826`) are user-confirmed; only backup command completion and existence of `secrets.yaml` were directly demonstrated, not a comprehensive contents or restore test.
- Which GitHub branches or PRs every current node actually references.
- Whether the forked Snapclient logging fix is required by the intended newer release.
- Whether all active YAML configurations are represented in GitHub.

**Do not ask the owner to learn branches or rebuild the old setup just to answer these questions.** Inspect the files, explain the few key dependencies in ordinary language, and take one controlled step at a time.
