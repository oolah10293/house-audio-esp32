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
