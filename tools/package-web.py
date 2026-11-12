"""Stage the versioned browser contract and hash every runtime artifact."""
from pathlib import Path
import argparse, shutil, hashlib, json, subprocess
parser=argparse.ArgumentParser()
parser.add_argument('--build',type=Path,default=Path('build/web'))
parser.add_argument('--output',type=Path,default=Path('dist/web'))
args=parser.parse_args()
files=['index.html','index.js','index.wasm','index.data','bridge.js']
for name in files:
    if not (args.build/name).is_file(): raise SystemExit(f'Missing build artifact: {args.build/name}')
args.output.mkdir(parents=True,exist_ok=True)
records={}
for name in files:
    destination=args.output/name
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
manifest={'schemaVersion':1,'buildId':build_id,'commit':revision,'protocolVersion':1,'entry':'index.html','files':records}
# Write the manifest last; consumers verify every hash before activating a revision.
(args.output/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print(f'Staged {len(files)} runtime files and manifest in {args.output.resolve()} ({build_id})')
