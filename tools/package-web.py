"""Stage the complete static browser runtime, without CMake internals."""
from pathlib import Path
import argparse, shutil
parser=argparse.ArgumentParser()
parser.add_argument('--build',type=Path,default=Path('build/web'))
parser.add_argument('--output',type=Path,default=Path('dist/web'))
args=parser.parse_args()
files=['index.html','index.js','index.wasm','index.data','bridge.js']
for name in files:
    if not (args.build/name).is_file(): raise SystemExit(f'Missing build artifact: {args.build/name}')
args.output.mkdir(parents=True,exist_ok=True)
for name in files: shutil.copy2(args.build/name,args.output/name)
print(f'Staged {len(files)} files in {args.output.resolve()}')
