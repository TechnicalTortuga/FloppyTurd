import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('web_assets',ROOT/'tools/prepare-web-assets.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
TEMP_PARENT=ROOT/'build/package-tests'
TEMP_PARENT.mkdir(parents=True,exist_ok=True)

class AssetSplit(unittest.TestCase):
    def test_bootstrap_excludes_music_and_level_dependencies(self):
        with tempfile.TemporaryDirectory(dir=TEMP_PARENT) as directory:
            root=Path(directory); source=root/'source'; output=root/'output'
            names=['fonts/pixel.ttf','mainmenu/menu.png','mainmenu/song.mp3','music/Level1.mp3','environment/Level1Clouds.png','enemies/RatCopterIdle.png','environment/SnowLevelBackground.png','enemies/SnowManThrow.png','objects/Snowball.png']
            for name in names:
                path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(name.encode())
            module.prepare(source,output)
            catalog=json.loads((output/'deferred-assets.json').read_text())
            self.assertEqual(set(p.relative_to(output/'bootstrap/resources').as_posix() for p in (output/'bootstrap/resources').rglob('*') if p.is_file()),{'fonts/pixel.ttf','mainmenu/menu.png'})
            self.assertIn('resources/environment/Level1Clouds.png',catalog['groups']['level-0'])
            self.assertIn('resources/enemies/RatCopterIdle.png',catalog['groups']['level-4'])
            self.assertIn('resources/enemies/RatCopterIdle.png',catalog['groups']['level-5'])
            self.assertIn('resources/objects/Snowball.png',catalog['groups']['level-3'])
            self.assertNotIn('resources/music/Level1.mp3',catalog['groups']['level-0'])
            for record in catalog['files'].values(): self.assertTrue((output/record['url']).is_file())
            self.assertIn('window.floppyAssetCatalog=',(output/'asset-catalog.js').read_text())
            (source/'environment/Level1Clouds.png').unlink()
            module.prepare(source,output)
            updated=json.loads((output/'deferred-assets.json').read_text())
            self.assertNotIn('resources/environment/Level1Clouds.png',updated['files'])

if __name__=='__main__': unittest.main()
