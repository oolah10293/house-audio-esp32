"""Verify newer baseline, HA controls and final linker interposition."""
from pathlib import Path
import re
import subprocess
import yaml

text = Path("tests/compile_bose_sub.yaml").read_text()
config = yaml.safe_load(text)
assert "snapclient" not in config, "obsolete top-level snapclient block"
assert config["media_player"][0]["platform"] == "snapclient"
assert config["wifi"]["post_connect_roaming"] is False
assert config["api"]["reboot_timeout"] == "0s"
assert config["external_components"][0]["source"] == (
    "github://luar123/esphome@4d3280bd35fdd970e628a22197f19ab0fded1a39"
)
assert "c-MM" not in text
assert {n["id"] for n in config["number"]} == {
    "bose_attenuation",
    "sub_lowpass",
    "sub_lowcut",
}
assert {s["id"] for s in config["switch"]} == {
    "sub_phase_invert",
    "sub_crossover_bypass",
}
log = Path("/tmp/bose-build.log").read_text()
assert "mdns version conflict" not in log, "legacy mDNS dependency override returned"

elf = next(Path("tests/.esphome").rglob("firmware.elf"))
packages = Path.home() / ".platformio/packages"
nm = next(p for p in packages.rglob("*-nm") if "xtensa" in p.name and p.is_file())
objdump = nm.with_name(nm.name.removesuffix("nm") + "objdump")
symbols = subprocess.check_output([str(nm), str(elf)], text=True)
for name in ("__wrap_dsp_processor_worker", "dsp_processor_worker"):
    assert re.search(r"\b[Tt]\s+" + name + r"\s*$", symbols, re.M), name + " missing from ELF"

# GNU ld --wrap deliberately rewrites upstream call sites to the wrapper.
# The native hook test proves that the wrapper calls __real exactly once; the
# final ELF check only needs to prove that real upstream audio invokes wrapper.
assembly = subprocess.check_output([str(objdump), "-d", str(elf)], text=True)
wrapper_calls = re.findall(
    r"^.*\bcall(?:0|4|8|12)\b[^\n]*<__wrap_dsp_processor_worker>.*$",
    assembly,
    re.M,
)
assert wrapper_calls, "No actual upstream call site for __wrap_dsp_processor_worker"
print("\n".join(wrapper_calls))
print(
    "PASS: PR14389 pinned; HA controls compiled; no legacy mdns override; "
    "upstream PCM path calls DSP wrapper"
)
