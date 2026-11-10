# ESPHome local setup and upgrade handoff

Last reviewed: 2026-11-09. This is a **documentation-only** handoff. It does not authorize updates, builds, firmware uploads, merges, or replacement of a working installation.

## Why this note exists

The owner runs ESPHome for several ESP32-based house-audio renderers and other devices. The local development setup was difficult to get working originally. Do not ask the owner to reconstruct installation history or GitHub branch history from memory when it can be recovered from the repositories and previous conversations.

**Practical priority:** Preserve the working Windows-based ESPHome installation, its launcher, and existing YAML configurations. Investigate an update in an isolated environment; do not make an in-place upgrade the first experiment.

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

- Current ESPHome version on the PC; exact `uv` environment; launcher file name/content; Windows path to local YAML.
- Which GitHub branches or PRs every current node actually references.
- Whether the forked Snapclient logging fix is required by the intended newer release.
- Whether all active YAML configurations are represented in GitHub.

**Do not ask the owner to learn branches or rebuild the old setup just to answer these questions.** Inspect the files, explain the few key dependencies in ordinary language, and take one controlled step at a time.
