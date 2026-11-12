import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
TEMP_PARENT=(ROOT/'build/package-tests').resolve()
if not TEMP_PARENT.is_relative_to(ROOT): raise RuntimeError('Invalid fixture root')
TEMP_PARENT.mkdir(parents=True,exist_ok=True)
FILES=['index.html','index.js','index.wasm','index.data','bridge.js']
class PackageContract(unittest.TestCase):
    def test_manifest_matches_complete_revision(self):
        with tempfile.TemporaryDirectory(dir=TEMP_PARENT) as directory:
            source=Path(directory)/'source'; output=Path(directory)/'output'; source.mkdir()
            for name in FILES: (source/name).write_bytes(('fixture '+name).encode())
            result=subprocess.run([sys.executable,str(ROOT/'tools/package-web.py'),'--build',str(source),'--output',str(output)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            manifest=json.loads((output/'build-manifest.json').read_text())
            self.assertEqual(manifest['schemaVersion'],1)
            self.assertEqual(manifest['protocolVersion'],1)
            self.assertEqual(manifest['entry'],'index.html')
            self.assertTrue(manifest['buildId'])
            self.assertEqual(set(manifest['files']),set(FILES))
            for name,record in manifest['files'].items():
                data=(output/name).read_bytes()
                self.assertEqual(record['bytes'],len(data))
                self.assertEqual(record['sha256'],hashlib.sha256(data).hexdigest())
    def test_missing_artifact_rejects_before_output(self):
        with tempfile.TemporaryDirectory(dir=TEMP_PARENT) as directory:
            source=Path(directory)/'source'; output=Path(directory)/'output'; source.mkdir()
            (source/'index.html').write_text('fixture')
            result=subprocess.run([sys.executable,str(ROOT/'tools/package-web.py'),'--build',str(source),'--output',str(output)],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertFalse(output.exists())
if __name__=='__main__': unittest.main()
