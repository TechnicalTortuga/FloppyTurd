"""Check registered runtime assets with exact filename case (also on Windows)."""
from pathlib import Path
import re, sys
root = Path(__file__).resolve().parents[1]
assets = {p.relative_to(root/'FloppyTurd/resources').as_posix() for p in (root/'FloppyTurd/resources').rglob('*') if p.is_file()}
source = (root/'FloppyTurd/ResourceManager.cpp').read_text(encoding='utf-8-sig')
missing = []
for match in re.finditer(r'RegisterResource\("([^"\n]+)",\s*"([^"\n]+)"', source):
    if match[2] not in assets: missing.append((match[1],match[2]))
for identifier, path in missing: print(f'{identifier}: missing or wrong case: {path}')
print(f'Checked {len(re.findall("RegisterResource",source))-1} registrations; {len(missing)} missing assets')
sys.exit(bool(missing))
