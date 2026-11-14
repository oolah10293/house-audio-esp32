# Lite-On CD input: mandatory delivery acceptance gate

This is a project checkpoint, not a claim that the firmware is fixed. It applies before any future Lite-On firmware delivery and must be revisited at each revision. Do not build or package without explicit user authorization.

## Filename and identity gate — mandatory

- Use the approved release's versioned Lite-On package name. **The latest installed CD firmware verified by Pi OTA is 3.3** (`LiteOn CD player 3.3`); package/version references to 2.0 below are historical. Keep `HouseMusic_CD_Pi.zip` as the separately named paired Pi integration package.
- Reject a delivered sketch if the actual ZIP lacks `bootSession` or `discRevision`, or contains the old webpage text `It does not automatically change House Music playback.`
- Put `VERSION.txt` in the package with the exact compiled commit SHA, feature identity and hardware-test status. Never substitute an old v0.2 sketch under a new ZIP name.
- Read the top of `README.md` and this gate before each future delivery. The GitHub CI workflow checks the ESP ZIP identity and name automatically.

## Source baseline (before modifying code)
- [ ] Read current branch, README, applicable CD documents, relevant source and latest commits.
- [ ] Identify last hardware-confirmed working revision and exact failing revision.
- [ ] Record concrete failure evidence; mark all hypotheses as unverified.
- [ ] Keep changes restricted to the failure being addressed; list all changed files.

## Verification
- [ ] Review complete diff against the intended parent; check unintended changes and restore paths.
- [ ] Run relevant unit/static checks and actual Arduino compile using pinned ESP32 Arduino core 2.0.14 / XIAO ESP32-S3.
- [ ] Capture CI run URL, result, exact HEAD SHA and which checked-out commit the run compiled.
- [ ] Do NOT equate compilation with physical CD testing; list hardware checks still pending.
- [ ] For CD work, preserve READ TOC, READ CD/WAV output, home Wi-Fi, Pi registration, and bootSession/discRevision unless deliberately changing their contract.
- [ ] Do not modify the live Pi unless necessary, explicitly authorized, and compatible with the tested S3 source.

## Packaging and delivery (only after explicit user authorization)
- [ ] Package from one pinned SHA; avoid moving-branch / in-progress artifact ambiguity.
- [ ] Inspect archive contents: complete Arduino sketch, USB/SCSI dependencies, transport header, placeholder config only. Do not include private credentials.
- [ ] Verify archive contents correspond to the same pinned source SHA that was compiled; compare hashes or reconstruct from pinned inputs.
- [ ] Verify download actually resolves as a usable file; link directly in chat.
- [ ] Provide exact commit, CI evidence, archive path, behavioral changes, and hardware status.
- [ ] Never describe a rollback, diagnostic, or compile pass as a resolved hardware fault.

## Open technical debt
- [ ] 1500 ms CBW/CSW timeout is **provisional**, 7.5× the old 200 ms. Test shorter reliable values once hardware functionality is confirmed; record evidence separately from concurrent reset-recovery changes. See USB_TIMEOUT_FOLLOWUP.md.
- [ ] Determine whether USB BOT recovery fixes intermittent 0x2 errors on real Lite-On hardware.
- [ ] Confirm disc detection, track enumeration, audio streaming, source automatic takeover and manual source override under real conditions.

## Current CD 3.3 hardware and delivery status (updated 2026-11-13)
- **OTA identity verified on hardware:** user installed 3.3 on the ESP32-S3 from the Pi; updater reported the expected compiled SHA `8fe9715801497399eafacd8331a9ccf1ee1e15b4` and running MD5 `0e0e4da19597308057924aaf8a65d770`.
- **Published package vs compiled revision verified:** authorized CI run `43489668762` compiled exact SHA and validated the packaged source and binary; publication run `43490039922` verified and published the existing image; one-command installer is pinned to its release ZIP SHA-256. Details: `CD_PLAYER_3_3_INSTALL.md`.
- **Playback result by user report:** track changes worked very well after installation, with a few timeout messages. Their origin and effect on audio are not yet independently identified; avoid claiming a timeout-free or long-duration acceptance pass.
- **Current branch commits after the compiled SHA are documentation/workflow/installer changes only**. Do not mistake branch HEAD for the compiled binary SHA. A future firmware code change requires a fresh, separately authorized build and exact-SHA verification.


## Historical build-gate setup record (2026-11-12)

The sections below document how the gate was developed and were written **before** the 3.3 compiled release. Their "not yet proven" status statements are historical; the verified 3.3 build and publication runs above supersede them.

## Automated enforcement status (2026-11-12)

- The updated `.github/workflows/arduino-cd-input-check.yml` removes automatic push-triggered builds and packages. It requires a manually authorized workflow invocation with the input `BUILD_AND_PACKAGE`.
- A successful job must compile on the pinned Arduino core and verify every ZIP file against the same checkout, with an embedded commit SHA and a SHA-256 manifest. Failing checks prevent artifact upload.
- **Not activated or proven:** GitHub requires a `workflow_dispatch` workflow to exist on the repository default branch for manual invocation. The current default `main` branch does **not** have this workflow. Therefore the new gate currently exists as source configuration on the development branch, not as a demonstrated working CI gate. Do not claim otherwise or distribute another release until the permitted workflow is made runnable and passed for the exact source revision.
- This workflow cannot enforce hardware acceptance, prevent someone manually attaching a different archive, or enforce the assistant's wording. Those remain separately documented release checks.
- Do not merge this into main, start any workflow, compile, or package without explicit user authorization for that work.


## Chat-controlled GitHub Actions trigger (2026-11-12)

- Active *configuration*, not yet end-to-end run-verified: the feature-branch workflow listens only for a push that changes `cd-arduino/ci/authorized-build-request.txt`. Ordinary firmware commits and documentation updates do not trigger it.
- The assistant must obtain explicit user authorization to build/package **in conversation first**. That is sufficient: no GitHub UI action is required from the user. Following authorization, update the marker to `BUILD_AND_PACKAGE <unique-request-id>` in a single-file commit. GitHub Actions then checks that the commit changed **only** the marker and that it has the authorized format, before compiling and packaging.
- The marker currently says `NOT_AUTHORIZED`. Setting up this trigger is not authorization to start a build. Do not edit the marker to the authorized format until authorization for that build is granted.
- A successful run must be verified against the exact marker commit SHA, its compile step, its archive validation, and its artifact. A GitHub Actions success alone does not mean physical hardware is tested.
- The assistant remains responsible for GitHub operations and source/package checks. The user owns physical installation, real-hardware testing, and acceptance. Never tell the user to navigate GitHub to start the process.
- **Not yet proven:** The new push-controlled dispatch has not been triggered, and no release artifact has been created through it.


## Verified trigger/deny test (2026-11-12)

- The assistant committed a **non-authorizing** marker update only, SHA `14f5de912f737a34e530f9048607c71eccdf4674`.
- GitHub automatically started run [43293617329](https://github.com/oolah10293/house-audio-esp32/actions/runs/43293617329) from the marker push, using the development-branch workflow.
- The checkout completed, then the **Enforce explicit authorization marker** step failed as expected.
- Crucially, the Arduino toolchain, compilation, ZIP validation, and artifact upload were **all skipped**. This verifies chat-controlled triggering and refusal to build without an approved marker.
- The *authorized happy path* remains untested pending the user's explicit authorization of a build/package. An untested happy path must not be described as passing.


## Cross-conversation USB/CD cleanup audit — 2026-11-13

These findings were recovered from the user's *previous full CD-player
conversation* as well as the current Pi/MPD troubleshooting, not only
from explicitly tagged TODOs. Pinned **installed and compiled 3.3**
source: `8fe9715801497399eafacd8331a9ccf1ee1e15b4`.
Later HEAD changes are documentation/delivery only.

**Not yet field-closed:**

1. The original intermittent **USB drive detected / disc not ready /
   audio tracks disappear** problem, including one user-reported case
   where restarting the ESP32 and manually rescanning did **not**
   recover CD tracks. Firmware includes non-ejecting SCSI START,
   temporary track-list retention, BOT reset/endpoint recovery and
   TOC refresh, but no controlled repeat failure-and-recovery hardware
   acceptance. Distinguish confirmed medium absence from raw USB
   transfer-status `0x2` using transfer-stage logs before attributing
   the error to a drive or cable.
2. The **1,500-ms USB CBW/CSW ceiling** remains provisional rather than
   measured; see `USB_TIMEOUT_FOLLOWUP.md`. It is not MPD's timeout,
   HTTPClient's separate 1,500-ms network timeout, or a fixed latency.
3. **Failed/short READ CD** retries (up to three attempts with 100-ms
   gaps) are implemented in the exact compiled 3.3 source, but
   resilience against a read error hasn't been demonstrated on real
   hardware. A terminal failure closes the WAV response with its
   announced Content-Length unmet; verify Pi gateway/MPD handling and
   subsequent successful track selections, rather than hiding faults
   behind fabricated PCM.
4. **SET CD SPEED** requests `706 kB/s` (~4×) on successful TOC.
   A successful command return does not prove actual sustained speed,
   and whether Lite-On honors the hint is unmeasured. Previous reports
   of mid-track MPD time stalling, looping playback time, and gateway
   502s were not independently closed by a complete-disc post-3.3
   test; measure real streamed throughput and uninterrupted audio.
5. **Physical eject/reinsert / different-disc changes** (especially
   missed rapid swaps and same-track-count discs), firmware-driven
   boot/autoplay, and source restoration have partial field evidence,
   not full hardware acceptance. The current implementation skips
   USB polls during active WAV streaming; test media changes when
   idle and during/after stream cancellation without degrading
   the working stream.
6. **Potential source edge case, not a field failure:** a syntactically
   valid TOC with zero recognized audio tracks returns false and keeps
   previously advertised tracks. That safely tolerates transient
   ambiguous failures, but a genuinely inserted data-only disc or
   missed media swap may leave stale audio entries until a confirmed
   medium-absent event. Reproduce before changing logic.
7. **Minor logging cleanup:** the compiled source's
   `SET CD SPEED` diagnostic at ~line 319 uses a literal
   escaped `\\n` in the source format string rather than a newline
   escape, potentially concatenating serial log lines. Address only
   in a separately authorized firmware revision; do not reflash
   just for cosmetic output.
8. **Documentation and release nomenclature:** the legacy
   `cd-arduino/README.md` documents `NetworkCDProbe` and explicitly
   says that diagnostic sketch does not stream; do not mistake it
   for the installed `HouseAudioCDInput` 3.3. Historical
   `CD_PLAYER_3_3_INSTALL.md` contains pre-install "uncompiled"
   text later superseded by the OTA acceptance section. The release
   workflow and acceptance gate also contain historical authorization
   policy descriptions, some superseded by the successfully used 3.3
   marker build; reconcile future-facing instructions only, not
   the factual timeline. Confirm intended version/filename for any
   future delivery from the user's latest instruction.

**Already confirmed:** 3.3 binary identity after authorized Pi OTA,
audible CD playback and substantially improved track switching. Later
Pi server stop-before-play fix received four HTTP 200 GUI selections.
That result does **not** prove full-disc reliability or erase the
USB error/timeout follow-up.

**Restrictions:** this audit is documentation-only. No firmware source
changes, compilation, build, ZIP packaging, publish, or OTA initiated.

## Cleanup item 2 — persistent USB connection but CD not ready (2026-11-13)

**Status: OPEN — baseline diagnostic pending.** This is item #2
in the sequential cross-repository CD cleanup. Item #1 (Pi
Next/Previous and track-1 boundary) is user-accepted and closed in
`oolah10293/house-audio-server` commit `a54f0253`.
Items #3–9 remain open and untouched.

**Original field evidence from earlier CD conversation** (also recorded
in `CD_SCSI_REGRESSION_REVIEW.md` and
`CD_STREAM_POLLING_SOURCE_CHECKPOINT_2026-11-12.md`):
USB device remained connected, but readiness, sense and non-ejecting
start commands sometimes returned `0x2`; track list could be empty.
An S3 restart restored Pi MP3 behavior, **not** CD tracks; a manual
rescan also failed. No serial log identifying exact BOT transfer
stage was supplied. Do not claim recovery already proved.

**Verified installed-source inspection:** compiled 3.3 commit
`8fe9715801497399eafacd8331a9ccf1ee1e15b4`, not the later
documentation-only branch head.
- `HouseAudioCDInput.ino` `discTask()` polls SCSI TEST UNIT READY
  every ~2 seconds when no active WAV lease owns the USB/SCSI device.
  A successful readiness response triggers TOC collection if first
  detection, recovery, user rescan or 15-second recheck. Failure calls
  REQUEST SENSE and distinguishes decoded medium-not-present sense
  from an unresolved transport error; no decoded removal means a
  former track list is normally retained.
- After 3 failed readiness polls the code attempts the non-ejecting
  START STOP UNIT command, at most once per ~30 seconds thereafter.
  This is **not** a physical drive power cycle or USB re-enumeration.
- USB Mass Storage BOT CBW/CSW failures invoke
  `usbhost_resetRecovery()` as applicable, attempting BOT reset
  and clearing endpoints. The 1,500-ms CBW/selected CSW ceiling is
  still provisional and belongs to item #7, not a proven readiness
  repair.
- `0x2` is compatible with a raw USB transfer timeout,
  **not proof** of SCSI sense key 02 or absent disc. Without stage
  logs, do not identify a cable, power supply, drive, S3 host or
  firmware scheduling as the established root cause.
- The existing local status page at `http://192.168.1.175/`
  exposes Status, SCSI diagnostic, consecutive failures, automatic
  drive-start count, boot session, disc revision and track list.
  A single healthy snapshot cannot prove the old intermittent bug
  is gone.

**Next minimal non-disruptive baseline:** With the CD/drive powered
as presently configured, read the ESP32 status page from the Pi
using `curl`; capture status, diagnostic, counters and track list.
If currently healthy, record this as a baseline and choose a controlled
recovery test rather than pretending spontaneous failure was
reproduced. If unhealthy, compare failure text/counters before and
after one manual rescan, with no firmware or power change.
Do not trigger new USB timeout tuning, firmware build, package, OTA
or Pi deployment under this item without separate user authorization.

## Cleanup item 2 — live 3.3 readiness baseline (2026-11-13)

**User-supplied device status output** (existing ESP32-S3 status page,
via Pi; not a fresh independent ESP32 serial trace):

```text
Status: Audio CD detected
Firmware: 3.3
Source commit: 8fe9715801497399eafacd8331a9ccf1ee1e15b4
SCSI diagnostic: Drive recovered; TOC refreshed
Consecutive not-ready checks: 0
Automatic drive-start attempts: 1
Disc revision: 1
Track 1 ... Track 13
Pi registration: OK (HTTP 200)
```

**What this establishes:** Firmware identifies 13 audio tracks
and successfully registers with the Pi at the observation.
The displayed recovery diagnostic is emitted by the exact installed
3.3 sketch when an earlier `TEST UNIT READY` polling failure occurred
(`failures > 0`) and subsequent readiness plus TOC read succeeded.
The nonzero automatic drive-start counter means this discTask
session attempted the non-ejecting START command once; the result
does not establish that this particular command *caused* successful
recovery. Zero consecutive failures indicates the poll failure count
was reset by successful readiness. `discRevision: 1` records a disc
identity observed; it is not the number of retries or recovery events.
Likewise the status line does not reveal whether any prior error
was USB timeout `0x2`, spin-up latency, or another response.

**Distinguish from original fault:** A transient not-ready condition
was recovered during the observed session, **but** the old persistent
device-connected/no-tracks problem (which survived ESP32 reboot and
manual rescan in an earlier firmware) was not reproduced here.
Do not mark the original fault cured or claim BOT recovery's cause
is isolated. Successful detection is a healthy current baseline,
not a negative long-duration test.

**Next least-disruptive question:** Determine whether the
connected-but-no-tracks condition has recurred at all on firmware 3.3.
If not, treat as *not observed yet on 3.3*, leave diagnostic
watchpoint open, and don't forcibly perturb the working drive to
simulate a different failure. If it has recurred, immediately capture
status/diagnostic/counters at failure before reboot/rescan; then run
one controlled recovery check. #3–9 remain open, untouched.
No code modifications, firmware build, packaging, flashing, or
deployment authorized or performed in this checkpoint.

## Cleanup item 2 — disposition after 3.3 field report (2026-11-13)

**User confirmation:** In direct response to whether the old
USB-connected/CD-tracks-disappear failure has recurred since installing
firmware 3.3, user answered **"i have not"**.

**Conclusion for sequential cleanup:** Item #2's *active investigation*
is **complete / DEFERRED TO MONITORING**, not a verified firmware repair.
The 3.3 status page currently recognizes all 13 audio tracks, shows
successful Pi registration (HTTP 200) and reports a recovered readiness
episode with zero consecutive failures and one drive-start attempt.
The user has **not observed the original persistent failure on 3.3**.
There is no reproduced persistent 3.3 fault to debug right now,
and intentionally disturbing the working USB drive would not
demonstrate recovery from the historical failure.

**Preserved unresolved risk:** The historical connected-but-no-tracks
failure previously survived S3 restart and manual rescan; without
failure-stage serial diagnostics, its USB/SCSI root cause and
whether 3.3 permanently fixes it remain **unverified**. Do not
equate this transient successful recovery with reproduction of that
specific persistent fault. Keep this watchpoint in the backlog.

**If the problem recurs:** Before rebooting or rescanning, capture
the S3 `http://192.168.1.175/` status, full SCSI diagnostic,
consecutive-not-ready count, drive-start attempt count,
disc revision, track list, boot session and Pi registration.
If available, gather USB serial messages identifying CBW/data/CSW
failure stage. Then evaluate one bounded recovery action at a time.

**Next:** Proceed to cleanup item #3 as a separate investigation;
all later items remain open. No firmware edit, build, package,
installer, OTA or Pi deployment was authorized or performed here.

## Cleanup item 3 — valid data-only TOC versus ambiguous TOC (2026-11-13)

**Observed? No.** This is a **source-verified edge case**, not a
user-reported data-only-disc failure on running firmware 3.3.
Compiled/running revision remains `8fe9715801497399eafacd8331a9ccf1ee1e15b4`.

**Original source behavior:** `HouseAudioCDInput.ino` obtains a valid
READ TOC table, parses audio descriptors into `fresh[]`, and when
`count == 0` unconditionally retains `discTracks[]` from the last
audio CD. This is intentional protection against ambiguous/failed
TOC observations, but on a legitimately data-only CD, previously
advertised audio numbers could remain visible on the node, be
registered with the Pi and lead to invalid CD track selections.
The present TOC parser alone proves the potential state; no
physical reproduction was claimed.

**Small source-only policy correction (NOT compiled or flashed):**
- `f75fb92d4f04783268f624ee80426a0ed6c251ad` changes
  `readDiscToc()` to report whether it saw audio, a pending
  valid data-only candidate, confirmed data-only, or a retryable
  ambiguous/failed result. A candidate data-only TOC requires
  a properly bounded table, ordinary data-track descriptors
  with increasing LBA ranges and a lead-out descriptor.
- A first well-formed all-data TOC keeps the last known-good
  audio list. The next idle poll (~2s) re-reads the TOC;
  only a **second identical successful all-data read** clears
  the old tracks, records the new disc identity/revision,
  and reports `Data CD detected; no audio tracks`.
- Transport errors, malformed/zero-audio ambiguous results and
  successful audio TOCs do not incorrectly confirm data-only.
  Normal audio detection and 15-second periodic TOC polling remain.
- `b1c9e9fcbcc09ed06c2949b5b63238080cff05ab` explicitly
  resets the data-only candidate on a failed TEST UNIT READY,
  so two reads separated by an unready event are not treated
  as consecutive evidence.
- The existing Pi registration emits `status: no_disc`
  and `tracks: []` when the confirmed data-only disc has zero
  audio tracks. This is a protocol limitation: `no_disc`
  currently means **no playable audio CD**, not necessarily
  physically empty tray. Do not change Pi behavior under #3.

**Static review only:** Current GitHub source was re-fetched after
the edits and eight source-structure checks were performed for
the TOC confirmation, valid data descriptors/lead-out, state
reset, next-poll recheck, and audio path preservation. This
is not a compiled firmware test, behavior simulation, GitHub
CI compile, or hardware evidence. The user has *not*
authorized a new build, installer, OTA or firmware deployment.

**Hardware acceptance after a separately authorized firmware
build/installation:** Verify audio CD => data-only CD clears
the old track list and never advertises stale tracks after
confirmation; repeat data-only polling remains empty; data-only
CD => audio CD restores the correct tracks and increments
revision; a single invalid/failed or partial TOC retains the
previous good audio disc; repeat normal audio-CD playback.
Do not claim #3 fixed on the running 3.3 until these tests pass.

**Disposition:** Source-level mitigation staged; runtime
acceptance **OPEN / untested**. No further alteration of
working 3.3 required today. Items #4–9 remain open.

## Cleanup item 3 — authorized compile/package/release (2026-11-13)

The user replied "authorized" after item #3 source-only
TOC/data-only disc mitigation was staged, authorizing a build/package
for hardware acceptance. This authorization was not treated as
permission to install/flash the ESP32 remotely.

**Verification completed, not inferred:**
- Compile and package from exact SHA
  `f7741b4c9faf160e84c8ca9fb5dde6da7989d59c`:
  https://github.com/oolah10293/house-audio-esp32/actions/runs/43546527447
  all steps PASSED, pinned Arduino-ESP32 2.0.14.
- The released image was taken from that **existing** successful build,
  not recompiled: publishing job PASS
  https://github.com/oolah10293/house-audio-esp32/actions/runs/43546807420 .
- GitHub release tag `liteon-cd-3.4` target = compiled SHA.
  Asset `LiteOn.CD.player.3.4.zip`, 547848 bytes;
  SHA-256 `ec0c620c1d24138dc534a5f9c01ff44db16cbc18d7d5550ed0729f12da50fcad`.
  Binary `HouseAudioCDInput.ino.bin`: 800192 bytes,
  SHA-256 `bd402601568a3ed0b35e8b4210fed97d0916913393d30d145ec69f5844fdab1c`.
  Verified downloaded Actions artifact ZIP and internal ZIP
  contents, exact source, files and BIN hashes.
- One-command Pi installer `cd-arduino/tools/install-liteon.sh`
  now points at the verified 3.4 release and exact ZIP SHA-256.
  Complete deliverable instructions and acceptance criteria:
  `cd-arduino/docs/CD_PLAYER_3_4_INSTALL.md`.

**Status:** #3 source fix = COMMITTED, COMPILED, PACKAGED AND
PUBLISHED. **Hardware acceptance NOT VERIFIED, still OPEN**.
The last user-confirmed device is 3.3; no Pi OTA or S3 flash
performed by the assistant. Verify audio CD -> data-only CD clears
old tracks; data-only -> audio CD restores correct list; normal
playback remains sound before accepting #3. No unrelated #4-9
work was authorized by this approval.
