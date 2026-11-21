# ESPHome local setup and upgrade handoff

Last reviewed: 2026-11-20. This is a **documentation-only** handoff. It does not authorize updates, builds, firmware uploads, merges, or replacement of a working installation.

## Why this note exists

The owner runs ESPHome for several ESP32-based house-audio renderers and other devices. The local development setup was difficult to get working originally. Do not ask the owner to reconstruct installation history or GitHub branch history from memory when it can be recovered from the repositories and previous conversations.

**Practical priority:** Preserve the working Windows-based ESPHome installation, its launcher, and existing YAML configurations. Investigate an update in an isolated environment; do not make an in-place upgrade the first experiment.

## Assistant operating boundary and communication correction — 2026-11-20

The owner has repeatedly corrected two distinct assistant behaviors: changing firmware source/configuration before receiving explicit **implementation** authorization, and repeatedly announcing obvious capability limitations (such as not being able to write to the owner's PC or flash a device) when they are not relevant to the answer. The latter mistake has been called out **at least five times**. These instructions are intended to prevent recurrences.

- **Don't volunteer capability disclaimers:** When the owner requests a Windows PowerShell command, provide the command directly. Do not preface it with "I can't access your PC," or follow it with a gratuitous statement that you did not flash, upload, build or otherwise execute it. The owner knows the division of work. Disclose a real limitation only when it materially prevents completing the requested task or changes what the answer means.
- **Don't narrate what was not done:** Report actions and actual results, with evidence levels. Avoid boilerplate about operations the assistant was never asked to perform or cannot perform.
- **GitHub is the assistant's maintenance responsibility:** Inspect repositories, update documentation and progress checkpoints, and commit already-authorized work without repeatedly asking permission for routine repository maintenance.
- **Firmware decisions belong to the owner:** A description of desired source-code, YAML, device-logic, dependency, or operating-setting changes is a specification, **not** authorization to implement. After inspection, discuss the precise proposed firmware changes and await an unmistakable implementation go-ahead. Implement only that scope; don't silently add improvements.
- **Keep answers operational:** Lead with the actual answer, command or status. Minimize ceremonial reassurances, explanations of well-known capabilities, and repetitions of authorization policies. If a step needs approval, ask only about that actual step.
- **Verified Windows executable:** In command examples, prefer the previously observed ESPHome interpreter at `C:\Users\John\AppData\Roaming\uv\tools\esphome\Scripts\python.exe` (or `$env:APPDATA\uv\tools\esphome\Scripts\python.exe`); do not assume that global Python has ESPHome.

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

## Final five-node functional acceptance — owner-confirmed 2026-11-20

**Milestone: PASS for all five.** The owner reported "synchronization and volume control confirmed on the remaining 4" after a four-device OTA upload batch returned PASS. Together with the earlier Basement stereo field report ("it flashed and started playing again" and "synchronized and snapcast volume control is working"), this establishes the following **owner-reported** five-node acceptance results, not independently measured telemetry:

| Device | IP | Config/compile | OTA | Audio/sync/volume |
| --- | --- | --- | --- | --- |
| Motorola Radio (C6-3) | `192.168.1.161` | PASS | PASS | PASS by owner report |
| Emerson Radio (C6-4) | `192.168.1.179` | PASS | PASS | PASS by owner report |
| Basement stereo (S3-1) | `192.168.1.117` | PASS | PASS | PASS by owner report |
| Music Room stereo (S3-2) | `192.168.1.100` | PASS | PASS | PASS by owner report |
| Garage stereo (S3-5) | `192.168.1.124` | PASS | PASS | PASS by owner report |

This **closes five-node initial functional rollout acceptance**. The four latest reports confirm synchronization and Snapcast volume behavior; audible sound is implied by the reported synchronization, but no individual timing measurements or device logs were supplied. **Not yet established:** long-term dropout-free stability, performance under heavier network load, and successful remote WARN log capture. These remain separate reliability/diagnostics follow-ups. Keep the existing backup, staged firmware binaries and local configuration until there is a clear reason to retire them. Do not repeat compilation or OTA for this milestone.

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

## Rio and Bose staged YAML configuration validation — owner-reported 2026-11-20

The owner downloaded the current GitHub `main` YAMLs as `C:\esphome\staging\rio-subwoofer.yaml` and `C:\esphome\staging\bose-subwoofer.yaml`. Windows PowerShell reported file sizes 4,589 and 6,532 bytes respectively. These byte counts are owner-supplied staging observations, not local file hash verification.

The owner then executed `$env:APPDATA\uv\tools\esphome\Scripts\python.exe -m esphome config` against each staged YAML, using ESPHome **2026.4.0**, and shared the command output.

| Device | Configuration result | Evidence |
|---|---|---|
| Rio Subwoofer | **PASS** | `INFO Configuration is valid!`, `PASS: rio-subwoofer.yaml` |
| Bose Sub | **PASS** | `INFO Configuration is valid!`, `PASS: bose-subwoofer.yaml` |

For both, the resolved ESPHome configuration showed `logger.level: WARN` and `logger.baud_rate: 0`. Bose and Rio reported `house_sub_dsp.lowpass_hz: 80.0`, with low-cut sliders allowing 0–80 Hz in 5 Hz steps. Both retained the pinned `luar123/esphome` Snapclient source at `4d3280bd35fdd970e628a22197f19ab0fded1a39`, and distinct additional components at their existing pinned revisions. Bose's independent hardware attenuation and standby controls were represented in the resolved configuration. The expanded configuration still lists `hardware_uart: USB_SERIAL_JTAG` as a default, but `baud_rate: 0` disables serial logger output; the Bose 4800-baud amplifier UART is separately configured.

Both validation commands emitted the non-fatal warning `cv.only_with_esp_idf was deprecated in 2026.1, will change behavior in 2026.6`. This warning did not prevent validation on ESPHome 2026.4.0. It is a future compatibility consideration, **not** authorization to change the pinned external components.

**Scope of evidence:** The shared output establishes that both staged YAML configurations parsed and passed ESPHome validation in the owner's environment. It does not establish executable compatibility, runtime audio behavior, or the outcome of any later workflow. No firmware-source edits were made to reach this checkpoint.

## Rio and Bose staged compilation — owner-reported PASS (2026-11-20)

Following the two successful `esphome config` validations, the owner reported **"pass"** in response to the supplied PowerShell loop invoking `$env:APPDATA\uv\tools\esphome\Scripts\python.exe -m esphome compile` for `C:\esphome\staging\rio-subwoofer.yaml` and `C:\esphome\staging\bose-subwoofer.yaml`.

**Owner-reported outcome:** Both Rio and Bose compile commands passed. This conclusion is based on the owner's brief PASS response; separate compiler output, artifact paths/sizes/hashes, and device behavior have not yet been inspected. No additional firmware-source or dependency changes are implied by this report.

**Next verification:** Read-only check that both new `firmware.bin` artifacts exist under the staging ESPHome build directories. Preserve the five-node staged configurations and existing working configuration folder. Any installation or acceptance checks are separate decisions after artifacts are verified.

## Rio and Bose staged firmware artifact inventory — owner-reported 2026-11-20

After owner-reported successful ESPHome 2026.4.0 compilation of both staged YAMLs, the owner ran a PowerShell recursive lookup of `firmware.bin` under `C:\esphome\staging\.esphome\build` and supplied this file inventory:

| Device | Reported binary file | Reported size (bytes) |
|---|---|---:|
| Bose | `C:\esphome\staging\.esphome\build\bose-sub\.pioenvs\bose-sub\firmware.bin` | 1,018,240 |
| Rio | `C:\esphome\staging\.esphome\build\house-audio-rio-sub\.pioenvs\house-audio-rio-sub\firmware.bin` | 996,608 |

**Evidence level:** Exact paths and byte sizes are from the owner's pasted command output, not direct filesystem access or a binary hash/content inspection. This closes the staged-file presence/size check; it is not a record of hardware acceptance. Preserve existing local configuration files and the previously accepted five-node staging artifacts.

## Rio and Bose target address inventory — owner screenshot, 2026-11-20

The owner supplied a device listing screenshot identifying the intended local deployment targets:

| Device label in screenshot | IPv4 address |
|---|---|
| `bose-sub` | `192.168.1.110` |
| `house-audio-rio-sub` | `192.168.1.131` |

The screenshot establishes the address-to-device label mapping at the time shown. It is not a live connectivity test or confirmation of a successful update. Compiled artifacts and configuration validation are recorded in the preceding checkpoints; runtime/OTA outcomes remain to be reported separately.

## Rio and Bose OTA upload — owner-reported PASS (2026-11-20)

The owner executed the supplied PowerShell loop that calls `esphome upload <staged YAML> --device <target IPv4> --file <existing firmware.bin>` for Rio followed by Bose, and reported **"both passed."** This is owner-reported success of the OTA command for both target devices, not independent access to command logs or post-update audio telemetry.

| Device | Target IP | OTA command status |
|---|---|---|
| Rio Subwoofer | `192.168.1.131` | **PASS (owner report)** |
| Bose Sub | `192.168.1.110` | **PASS (owner report)** |

**Functional acceptance still pending:** Audible bass, synchronization with the house speakers, Rio follow/mute behavior when assigned speakers leave and return, Bose amplifier acknowledgement and standby/awake behavior, and restored tuning controls. No firmware-source edits are implied by this checkpoint.

## Rio and Bose playback and synchronization — owner-confirmed PASS (2026-11-20)

Following successful OTA commands on both subwoofers, the owner answered **"they are"** to the explicit question **"Are both subwoofers playing and synchronized?"**. This confirms, by owner field report, **audible playback and synchronization PASS** for both Rio and Bose with the house audio system.

| Device | Audible playback | Synchronization |
|---|---|---|
| Rio Subwoofer | **PASS — owner confirmed** | **PASS — owner confirmed** |
| Bose Sub | **PASS — owner confirmed** | **PASS — owner confirmed** |

This does **not** establish Rio follow/mute transitions, Bose standby/wake acknowledgment or forced modes, restored Home Assistant tuning persistence, or long-term dropout-free stability. Those are separate functional checks. No firmware-source changes were made for this checkpoint.

## Rio automatic mute and Bose delayed standby — owner-confirmed PASS (2026-11-20)

The owner confirmed **"it does"** in response to the question: **"When the last assigned main speaker stops playing, does Rio mute automatically and Bose enter standby after its 30-second delay?"** Record these as **PASS by owner field report**:

- **Rio:** Automatic follow/mute behavior when the last assigned main speaker stops playing.
- **Bose:** Automatic standby after the configured 30-second delay under the same condition.

The prior owner report separately confirmed playback and synchronization of both subwoofers. The present response does not separately verify Bose wake acknowledgment or Rio unmute after return, forced Bose modes, persistence of tuning settings, or long-term reliability; those remain untested or unreported. No firmware source change was made for this record.
