#!/usr/bin/env python3

from pathlib import Path

import json
import os
import shutil
import urllib.request

def download_package_files(repo, at, target_dir, *, lock=True):
    target_dir.mkdir(parents=True, exist_ok=True)

    for filename in 'package-lock.json', 'package.json':
        if not lock and 'lock' in filename:
            continue

        print(f'Downloading {repo}/{filename}...')
        url = f'https://github.com/LLK/{repo}/raw/{at}/{filename}'

        with urllib.request.urlopen(url) as src:
            with open(target_dir / filename, 'wb') as dst:
                shutil.copyfileobj(src, dst)

def collect_assets(output, library):
    for item in library:
        md5 = item.get('md5') or item.get('baseLayerMD5')
        assert md5, item
        output.add(md5)

LIBRARY_NAMES = [
    'backdrops',
    'costumes',
    'sounds',
    'sprites',
]

# XXX: Don't necessarily want to depend on a yaml parser
with open('edu.mit.Scratch.yaml') as fp:
    for line in fp:
        if 'https://github.com/LLK/scratch-desktop.git' in line:
            for line2 in fp:
                if 'tag' in line2:
                    desktop_tag = line2.split(':')[1].strip()
            break

print('scratch-desktop:', desktop_tag)

desktop_dir = Path('scratch-desktop')
download_package_files('scratch-desktop', desktop_tag, desktop_dir)

with open(desktop_dir / 'package-lock.json') as fp:
    package_lock = json.load(fp)

gui_commit = package_lock['dependencies']['scratch-gui']['version'].split('#')[1]
gui_dir = desktop_dir / 'node_modules' / 'scratch-gui'
download_package_files('scratch-gui', gui_commit, gui_dir, lock=False)

print('Reading assets...')

assets = set()

for name in LIBRARY_NAMES:
    print(f'Library {name}')
    url = f'https://github.com/LLK/scratch-gui/raw/{gui_commit}/src/lib/libraries/{name}.json'
    with urllib.request.urlopen(url) as fp:
        library = json.load(fp)

    collect_assets(assets, library)
    if name == 'sprites':
        for item in library:
            for nested in 'costumes', 'sounds':
                if nested in item['json']:
                    collect_assets(assets, item['json'][nested])

sources = []
for asset in assets:
    sources.append({
        'type': 'file',
        'url': f'https://cdn.assets.scratch.mit.edu/internalapi/asset/{asset}/get/',
        'md5': os.path.splitext(asset)[0],
        'dest': 'static/assets',
        'dest-filename': asset,
    })

with open('asset-sources.json', 'w') as fp:
    json.dump(sources, fp, indent=4)

print('Wrote asset-sources.json.')
