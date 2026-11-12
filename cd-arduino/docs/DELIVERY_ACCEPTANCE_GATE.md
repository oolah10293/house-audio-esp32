# Lite-On CD input: mandatory delivery acceptance gate

This is a project checkpoint, not a claim that the firmware is fixed. It applies before any future Lite-On firmware delivery and must be revisited at each revision. Do not build or package without explicit user authorization.

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

## Delivery status for current branch
- **Not hardware verified.** Any previous CI run at a different commit does not validate the latest delivery SHA.
- **Previously delivered archive equivalence to compiled revision remains unverified**, and must not be claimed complete.
