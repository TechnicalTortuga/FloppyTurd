"""Split browser bootstrap resources from content-addressed, on-demand assets."""
from pathlib import Path
import argparse
import fnmatch
import hashlib
import json
import shutil

# Shared player/UI/SFX and the credits' plain pipes stay in the small bootstrap.
# Every level's transitive scenery, obstacle and enemy dependencies live together.
LEVEL_PATTERNS = [
    ['environment/Level1*.png'],
    ['environment/*[Ss]ewer*.png', 'environment/*PipeWide*.png', 'objects/Janitor*.png', 'enemies/Toilet*.png', 'enemies/toiletpaper*.png', 'sounds/ios/tpkill.mp3'],
    ['environment/Level3*.png', 'environment/Cacti.png', 'environment/dancing*.png', 'environment/Outhouse*.png', 'objects/Cacti*.png', 'objects/BrickWall.png', 'enemies/Bird*.png', 'sounds/ios/birdkill.mp3'],
    ['environment/Snow*.png', 'environment/snow*.png', 'environment/*ToiletSnow.png', 'enemies/Snow*.png', 'objects/Snowball.png'],
    ['environment/castle*.png', 'environment/curtains.png', 'environment/*ToiletGold*.png', 'environment/TorchPillar.png', 'objects/*Painting.png', 'objects/SpikeBall*.png', 'enemies/RatCopter*.png', 'sounds/ios/ratkill.mp3'],
    ['environment/Boss*.png', 'environment/bosspillar.png', 'environment/ratkingbackground.png', 'environment/darkclouds.png', 'environment/screenCurtains.png', 'enemies/Ratking*.png', 'enemies/RatCopter*.png', 'enemies/Toilet*.png', 'enemies/toiletpaper*.png', 'music/BossBeatv2.mp3', 'sounds/ios/ratkill.mp3', 'sounds/ios/ratkinghurt.mp3', 'sounds/ios/tpkill.mp3'],
]

def prepare(source, output):
    core = output/'bootstrap/resources'
    blobs = output/'assets'
    core.mkdir(parents=True, exist_ok=True)
    blobs.mkdir(parents=True, exist_ok=True)
    groups = {f'level-{i}': [] for i in range(6)}
    files = {}
    expected_core, expected_blobs = set(), set()
    for path in sorted(source.rglob('*')):
        if not path.is_file(): continue
        relative = path.relative_to(source).as_posix()
        memberships = [i for i, patterns in enumerate(LEVEL_PATTERNS)
                       if any(fnmatch.fnmatchcase(relative, pattern) for pattern in patterns)]
        music = relative.startswith('music/') or (relative.startswith('mainmenu/') and path.suffix in ('.mp3','.ogg'))
        if memberships or music:
            data = path.read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            name = digest + path.suffix.lower()
            target = blobs/name
            if not target.exists(): target.write_bytes(data)
            expected_blobs.add(target)
            virtual = 'resources/' + relative
            files[virtual] = {'url': 'assets/'+name, 'bytes': len(data), 'sha256': digest}
            for i in memberships: groups[f'level-{i}'].append(virtual)
        else:
            target = core/relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
            expected_core.add(target)
    # Reconfiguration must not leave removed/recategorized files in a bundle.
    for folder, expected in ((core, expected_core), (blobs, expected_blobs)):
        for path in folder.rglob('*'):
            if path.is_file() and path not in expected: path.unlink()
    catalog = {'schemaVersion': 1, 'files': files, 'groups': groups}
    encoded = json.dumps(catalog, sort_keys=True, separators=(',',':'))
    (output/'deferred-assets.json').write_text(encoded+'\n', encoding='utf8')
    # The running binary retains its own catalog across a host revision swap.
    (output/'asset-catalog.js').write_text('window.floppyAssetCatalog='+encoded+';\n', encoding='utf8')
    print(f'Web bootstrap: {sum(p.stat().st_size for p in expected_core):,} bytes; {len(files)} deferred resources')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.source, args.output)
