#!/usr/bin/env bash
# Downloads the core DBs (epitaka, dpd-dictionary) into assets/db so they
# are bundled into the app. The URLs come from the checked-in
# assets/translations_manifest.json (single source of truth) — the same
# logic as the "Download core DBs" step in .github/workflows/build_app.yml.
#
# Implemented with python3 (guaranteed in the freedesktop SDK) using only
# the standard library, so the build sandbox does not need curl/unzip.
set -e

python3 - <<'PY'
import json
import pathlib
import shutil
import urllib.request
import zipfile

core = json.load(open('assets/translations_manifest.json'))['core']
jobs = [
    ('epitaka', core['epitaka']['url']),
    ('dpd_dictionary', core['dpd_dictionary']['url']),
]

assets_db = pathlib.Path('assets/db')
assets_db.mkdir(parents=True, exist_ok=True)

for name, url in jobs:
    print(f'Downloading {url}')
    zip_path = f'/tmp/{name}.zip'
    urllib.request.urlretrieve(url, zip_path)
    with zipfile.ZipFile(zip_path) as z:
        z.extractall(f'/tmp/{name}_x')
    for db in pathlib.Path(f'/tmp/{name}_x').rglob('*.db'):
        shutil.copy2(db, assets_db / db.name)

for db in sorted(assets_db.glob('*.db')):
    print(f'  {db.name} ({db.stat().st_size} bytes)')
PY

ls -la assets/db/
