"Verify the exact shipped firmware, HA entities and final linker interposition."

from pathlib import Path
import re
import subprocess
import yaml


class ESPHomeLoader(yaml.SafeLoader):
    """Safe YAML loader that preserves ESPHome tags such as !lambda/!secret."""


def construct_esphome_tag(loader, node):
    if isinstance(node, yaml.ScalarNode):
        return loader.construct_scalar(node)
    if isinstance(node, yaml.SequenceNode):
        return loader.construct_sequence(node)
    if isinstance(node, yaml.MappingNode):
        return loader.construct_mapping(node)
    raise TypeError(f"Unsupported YAML node: {type(node)!r}")


ESPHomeLoader.add_constructor(None, construct_esphome_tag)

firmware_path = Path("firmware/bose-subwoofer-prototype.yaml")
text = firmware_path.read_text()
config = yaml.load(text, Loader=ESPHomeLoader)

assert "snapclient" not in config, "obsolete top-level snapclient block"
player = config["media_player"][0]
assert player["platform"] == "snapclient"
assert player["internal"] is True, "dead HA media-player controls must stay hidden"
assert config["wifi"]["post_connect_roaming"] is False
assert config["api"]["reboot_timeout"] == "0s"
assert config["external_components"][0]["source"] == (
    "github://luar123/esphome@4d3280bd35fdd970e628a22197f19ab0fded1a39"
)
assert config["external_components"][1]["source"] == (
    "github://oolah10293/house-audio-esp32@"
    "6fee4c6b3e0fbc3c9bb61ebce5ed67fd5a871278"
)
assert "c-MM" not in text

numbers = {entry["id"]: entry for entry in config["number"]}
assert set(numbers) == {
    "bose_attenuation",
    "sub_lowpass",
    "sub_lowcut",
}
assert numbers["bose_attenuation"]["min_value"] == 0
assert numbers["bose_attenuation"]["max_value"] == 40
assert numbers["bose_attenuation"]["initial_value"] == 12
assert numbers["sub_lowpass"]["min_value"] == 60
assert numbers["sub_lowpass"]["max_value"] == 160
assert numbers["sub_lowpass"]["initial_value"] == 90
assert numbers["sub_lowcut"]["min_value"] == 0
assert numbers["sub_lowcut"]["max_value"] == 50
assert numbers["sub_lowcut"]["step"] == 10
assert numbers["sub_lowcut"]["initial_value"] == 0
assert all(entry["restore_value"] is True for entry in numbers.values())
assert all(entry["entity_category"] == "config" for entry in numbers.values())

switches = {entry["id"]: entry for entry in config["switch"]}
assert set(switches) == {
    "sub_phase_invert",
    "sub_crossover_bypass",
    "bose_standby_test",
}
assert all(entry["entity_category"] == "config" for entry in switches.values())
standby = switches["bose_standby_test"]
assert standby["name"] == "Bose Standby (Test)"
assert standby["restore_mode"] == "ALWAYS_OFF"
assert standby["optimistic"] is True
assert "{0x01, 0x00, 0x80, 0x81}" in standby["turn_on_action"][0]["lambda"]
assert "id(bose_boot_complete)" in standby["turn_off_action"][0]["lambda"]
assert "const uint8_t wake[]" in standby["turn_off_action"][0]["lambda"]
assert "const uint8_t analog[]" in standby["turn_off_action"][0]["lambda"]
assert "const uint8_t stream[]" in standby["turn_off_action"][0]["lambda"]
assert "id(bose_ready) = true" in standby["turn_off_action"][0]["lambda"]
assert "bose_boot_complete" in {entry["id"] for entry in config["globals"]}

scripts = {entry["id"]: entry for entry in config["script"]}
assert set(scripts) == {
    "bose_send_attenuation",
    "sub_apply_lowpass",
    "sub_apply_lowcut",
}
for script in scripts.values():
    assert script["mode"] == "restart"
    assert script["then"][0] == {"delay": "150ms"}

log = Path("/tmp/bose-build.log").read_text()
assert "mdns version conflict" not in log, "legacy mDNS dependency override returned"

elf = next(Path("firmware/.esphome").rglob("firmware.elf"))
packages = Path.home() / ".platformio/packages"
nm = next(p for p in packages.rglob("*-nm") if "xtensa" in p.name and p.is_file())
objdump = nm.with_name(nm.name.removesuffix("nm") + "objdump")

symbols = subprocess.check_output([str(nm), str(elf)], text=True)
for name in ("__wrap_dsp_processor_worker", "dsp_processor_worker"):
    assert re.search(r"\b[Tt]\s+" + name + r"\s*$", symbols, re.M), (
        name + " missing from ELF"
    )

# GNU ld --wrap rewrites upstream call sites to the wrapper. The native hook
# test separately proves that the wrapper invokes the real worker once per
# fragment; this checks that the final firmware's PCM path invokes the wrapper.
assembly = subprocess.check_output([str(objdump), "-d", str(elf)], text=True)
wrapper_calls = re.findall(
    r"^.*\bcall(?:0|4|8|12)\b[^\n]*<__wrap_dsp_processor_worker>.*$",
    assembly,
    re.M,
)
assert wrapper_calls, "No upstream call site for __wrap_dsp_processor_worker"

print("\n".join(wrapper_calls))
print(
    "PASS: exact shipped YAML compiled from immutable source pins; six HA "
    "controls including test-only Bose standby/wake; dead media-player controls "
    "hidden; slider updates debounced; no legacy mdns override; PCM path calls DSP wrapper"
)
