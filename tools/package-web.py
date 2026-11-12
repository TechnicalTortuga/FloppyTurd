"""Stage the versioned browser contract and hash every runtime artifact."""
from pathlib import Path
import argparse, shutil, hashlib, json, subprocess
parser=argparse.ArgumentParser()
parser.add_argument('--build',type=Path,default=Path('build/web'))
parser.add_argument('--output',type=Path,default=Path('dist/web'))
args=parser.parse_args()
files=['index.html','index.js','index.wasm','index.data','bridge.js']
deferred=args.build/'deferred-assets.json'
if deferred.is_file():
    catalog=json.loads(deferred.read_text(encoding='utf8'))
    if catalog.get('schemaVersion') != 1: raise SystemExit('Unsupported deferred asset catalog')
    files.append('deferred-assets.json')
    for virtual,record in catalog['files'].items():
        path=Path(record['url'])
        if not virtual.startswith('resources/') or '..' in Path(virtual).parts or '\\' in virtual:
            raise SystemExit('Invalid virtual asset path')
        if path.is_absolute() or '..' in path.parts or len(path.parts)!=2 or path.parts[0]!='assets' or '\\' in record['url']:
            raise SystemExit('Invalid deferred asset URL')
        if path.stem!=record['sha256'] or len(record['sha256'])!=64:
            raise SystemExit('Asset URL is not content addressed')
        data=(args.build/path).read_bytes()
        if len(data)!=record['bytes'] or hashlib.sha256(data).hexdigest()!=record['sha256']:
            raise SystemExit('Deferred asset integrity check failed')
        files.append(path.as_posix())
    for paths in catalog['groups'].values():
        if any(path not in catalog['files'] for path in paths): raise SystemExit('Unknown group resource')
    files=sorted(set(files))
for name in files:
    if not (args.build/name).is_file(): raise SystemExit(f'Missing build artifact: {args.build/name}')
args.output.mkdir(parents=True,exist_ok=True)
records={}
for name in files:
    destination=args.output/name
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(args.build/name,destination)
    data=destination.read_bytes()
    records[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
root=Path(__file__).resolve().parents[1]
try:
    head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True).strip())
except (OSError,subprocess.CalledProcessError):
    head=None; dirty=True
revision=head if head and not dirty else None
identity=hashlib.sha256(json.dumps(records,sort_keys=True).encode()).hexdigest()[:12]
build_id=f'{"git-"+head[:12] if revision else "local"}-{identity}'
manifest={'schemaVersion':2 if deferred.is_file() else 1,'buildId':build_id,'commit':revision,'protocolVersion':1,'entry':'index.html','files':records}
if deferred.is_file(): manifest['deferredAssetsManifest']='deferred-assets.json'
# Drop stale generated files, while preserving no untracked distribution content.
for path in args.output.rglob('*'):
    if path.is_file() and path.relative_to(args.output).as_posix() not in records and path.name!='build-manifest.json':
        path.unlink()
# Write the manifest last; consumers verify every hash before activating a revision.
(args.output/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print(f'Staged {len(files)} runtime files and manifest in {args.output.resolve()} ({build_id})')
