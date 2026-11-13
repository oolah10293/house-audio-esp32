#!/usr/bin/env bash
# Pi-side installer for the already-compiled LiteOn CD player 3.2 firmware.
set -euo pipefail

HOST="${LITEON_HOST:-192.168.1.175}"
URL="https://github.com/oolah10293/house-audio-esp32/releases/download/liteon-cd-3.2/LiteOn.CD.player.3.2.zip"
EXPECTED_ZIP="8086352210cec9735b6e2c289f361abc683ce4400058d0a632a058568c1a4599"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

command -v curl >/dev/null || { echo "ERROR: curl not installed" >&2; exit 1; }
command -v python3 >/dev/null || { echo "ERROR: python3 not installed" >&2; exit 1; }
echo "LiteOn CD player 3.2 — Pi OTA installer"
echo "Downloading verified firmware from GitHub..."
curl --fail --location --silent --show-error --retry 2 "$URL" -o "$WORK/release.zip"
printf '%s  %s\n' "$EXPECTED_ZIP" "$WORK/release.zip" | sha256sum --check --status || {
  echo "ERROR: Firmware archive checksum mismatch. Nothing uploaded." >&2
  exit 1
}
python3 - "$WORK/release.zip" "$WORK" <<'PY'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as z:
    for name in ('HouseAudioCDInput.ino.bin','SOURCE_MANIFEST.json','update_cd_from_pi.py'):
        with open(sys.argv[2]+'/'+name, 'wb') as f:
            f.write(z.read(name))
PY
echo "Verified firmware archive. Starting OTA from this Pi..."
python3 "$WORK/update_cd_from_pi.py" "$WORK/HouseAudioCDInput.ino.bin" --manifest "$WORK/SOURCE_MANIFEST.json" --host "$HOST"
