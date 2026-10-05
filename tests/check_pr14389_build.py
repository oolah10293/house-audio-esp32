"""Check the baseline AND final linking, not just whether a build is green."""
from pathlib import Path
import re
import subprocess
import yaml

config = yaml.safe_load(Path('tests/compile_bose_sub.yaml').read_text())
assert 'snapclient' not in config, 'obsolete top-level snapclient block'
assert config['media_player'][0]['platform'] == 'snapclient'
assert config['wifi']['post_connect_roaming'] is False
assert config['external_components'][0]['source'] == 'github://luar123/esphome@4d3280bd35fdd970e628a22197f19ab0fded1a39'
assert 'c-MM' not in Path('tests/compile_bose_sub.yaml').read_text()
log = Path('/tmp/bose-build.log').read_text()
assert 'mdns version conflict' not in log, 'legacy mDNS dependency override returned'

elf = next(Path('tests/.esphome').rglob('firmware.elf'))
packages = Path.home()/'.platformio/packages'
nm = next(p for p in packages.rglob('*-nm') if 'xtensa' in p.name and p.is_file())
objdump = nm.with_name(nm.name.removesuffix('nm') + 'objdump')
symbols = subprocess.check_output([str(nm), str(elf)], text=True)
for name in ('__wrap_dsp_processor_worker', 'dsp_processor_worker'):
    assert re.search(r'\b[Tt]\s+' + name + r'\s*$', symbols, re.M), name + ' missing from ELF'
assembly = subprocess.check_output([str(objdump), '-d', str(elf)], text=True)
for name in ('__wrap_dsp_processor_worker', 'dsp_processor_worker'):
    calls = re.findall(r'^.*\bcall(?:0|4|8|12)\b[^\n]*<' + name + r'>.*$', assembly, re.M)
    assert calls, 'No actual call site for ' + name
    print('\n'.join(calls))
print('PASS: PR14389 media-player baseline pinned; no legacy mdns override; actual wrapper and original worker calls retained')
