# Lite-On USB timeout: provisional diagnostic setting

**Open follow-up — do not ship as an accepted permanent value without hardware evidence.**

The recovery branch `fix/liteon-usb-transport-recovery` raises USB Bulk-Only Transport CBW and relevant CSW timeouts from **200 ms to 1,500 ms**. This is a diagnostic change, **not** a validated optimal timeout or a permanent design decision.

- Current USB timeout behavior in `cd-arduino/HouseAudioCDInput/src/usb_host_msc/usbhost_msc_cmd.c`: CBW 1500 ms; CSW after a data phase 1500 ms; retry after stalled CSW 1500 ms. Other command-specific waits are unchanged.
- The user correctly flagged the **7.5× increase** as significant. Keep this concern open.
- Once the updated firmware is flashed, confirm physical CD detection, successful audio reads, intermittent failure behavior, and recovery. A successful result does **not** by itself establish that the timeout increase fixed the issue, since recovery handling was changed at the same time.
- Follow-up: isolate the effect of timeout versus transport recovery. If reliable, reduce the provisional wait (for example trial 500 ms), then verify startup, disc changes, WAV extraction, and repeated recovery under actual hardware conditions. If it still fails, diagnose USB transport instead of blindly extending timeouts.
- **Acceptance criterion:** shortest empirically reliable timeout with working automatic disc detection and audio streaming, documented with hardware observations. Until then, 1500 ms remains explicitly provisional.
- Do not compile, package, or release firmware without the user's explicit authorization for that work.
