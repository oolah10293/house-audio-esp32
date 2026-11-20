#!/usr/bin/env python3
"""Run ON THE HOUSE MUSIC PI. Upload a verified CD player firmware binary to S3."""
import argparse
import hashlib
import json
import pathlib
import time
import urllib.error
import urllib.request
import uuid

def get_status(host, timeout=3):
    with urllib.request.urlopen(f"http://{host}/api/firmware", timeout=timeout) as resp:
        return json.load(resp)

def main():
    parser = argparse.ArgumentParser(description="Pi -> LiteOn CD S3 OTA installer")
    parser.add_argument("firmware", type=pathlib.Path, help="HouseAudioCDInput.ino.bin")
    parser.add_argument("--host", default="192.168.1.175", help="ESP32-S3 address")
    parser.add_argument("--manifest", type=pathlib.Path, help="optional SOURCE_MANIFEST.json")
    args = parser.parse_args()

    if not args.firmware.is_file():
        parser.error("Firmware .bin does not exist")
    firmware = args.firmware.read_bytes()
    if len(firmware) < 100000 or len(firmware) > 2500000 or firmware[:1] != b"\xe9":
        parser.error("Not a plausible ESP32 application image")
    digest = hashlib.sha256(firmware).hexdigest()
    expected_sha = ""
    if args.manifest:
        manifest = json.loads(args.manifest.read_text())
        expected_sha = manifest["source_sha"]
        expected_bin_hash = manifest["firmware_bin_sha256"]
        if digest != expected_bin_hash:
            parser.error("Firmware SHA256 does not match build manifest")
    current = get_status(args.host)
    print("Currently running:", json.dumps(current, sort_keys=True))
    if expected_sha and current.get("sourceSha") == expected_sha:
        print("Source revision is already installed. No upload performed.")
        return

    boundary = "CDOTA" + uuid.uuid4().hex
    header = (
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="firmware"; filename="firmware.bin"\r\n'
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode()
    body = header + firmware + f"\r\n--{boundary}--\r\n".encode()
    print(f"Uploading {len(firmware)} bytes (SHA256 {digest}) from the Pi...")
    request = urllib.request.Request(
        f"http://{args.host}/update", data=body, method="POST",
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}",
                 "Content-Length": str(len(body))})
    with urllib.request.urlopen(request, timeout=120) as response:
        result = json.load(response)
    if not result.get("accepted"):
        raise RuntimeError(f"ESP32 did not accept update: {result!r}")
    print("Image accepted; waiting for reboot and firmware identity...")
    for i in range(45):
        time.sleep(2)
        try:
            installed = get_status(args.host)
            if expected_sha:
                if installed.get("sourceSha") == expected_sha:
                    print("PASS: ESP32 reports expected running commit:", expected_sha)
                    print("Running image MD5:", installed.get("sketchMd5"))
                    return
            elif installed.get("sketchMd5") != current.get("sketchMd5"):
                print("PASS: ESP32 reports a different running image:", json.dumps(installed))
                return
        except (OSError, TimeoutError, ValueError):
            pass
    raise SystemExit("No confirmed new image after reboot. Do not assume OTA succeeded.")

if __name__ == "__main__":
    main()
