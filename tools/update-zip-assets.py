#!/usr/bin/env python3

# Dependencies:
# - python-requests: https://docs.python-requests.org/en/latest/
# - python-ruamel-yaml: https://pypi.org/project/ruamel.yaml/

# To update the assets of an existing Flatpak manifest, run:
# ./update-zip-assets.py -m ../br.gov.fazenda.receita.irpf*.yaml -x -y <YEAR>

import argparse
import datetime
import hashlib
import os
import re
import requests
import sys
import xml.etree.ElementTree as ET
from ruamel.yaml import YAML

ASSETS_URL = 'https://downloadirpf.receita.fazenda.gov.br/irpf/{year:d}/irpf/update/{path:s}'
REMOTE_XML_FILE = 'latest.xml'

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def validate_year(year):
    year = int(year)
    if year < 2020:
        raise argparse.ArgumentTypeError('value must be equal to or greater than 2020.')
    return year

def manifest_exists(path):
    if not os.path.exists(path):
        raise argparse.ArgumentTypeError(f'flatpak manifest "{path}" not found.')
    return path

parser = argparse.ArgumentParser(description='Generate or update IRPF\'s zip assets.')
parser.add_argument('-m', '--manifest', type=manifest_exists,
                    help='Path to the Flatpak YAML manifest to be auto-updated')
parser.add_argument('-y', '--year', type=validate_year, default=datetime.datetime.now().year,
                    help='Year of the zip assets (defaults to current year)')
parser.add_argument('-x', action=argparse.BooleanOptionalAction,
                    help='Also generate x-checker-data entry')

args = parser.parse_args()

final_url = ASSETS_URL.format(year=args.year, path=REMOTE_XML_FILE)

eprint(f'Fetching remote XML file "{os.path.basename(final_url)}" ({args.year}) ... ', end='', flush=True)

try:
    r = requests.get(final_url)
except Exception as err:
    eprint('ERROR')
    eprint('Reason:', err)
    sys.exit(1)

if r.status_code != 200:
    eprint('ERROR')
    eprint('Reason: Unexpected HTTP response code:', r.status_code)
    sys.exit(1)

eprint('OK')

try:
    eprint('Parsing response body as XML ... ', end='', flush=True)
    root = ET.fromstring(r.text)
except:
    eprint('ERROR')
    eprint('Reason: Unable to parse the following data as XML:\n')
    eprint(r.text)
    sys.exit(1)

zip_assets = root.findall('.//extra/files/file')
total = len(zip_assets)

if total == 0:
    eprint('ERROR')
    eprint('Reason: Found no zip assets.')
    sys.exit(1)

eprint('OK')

count = 1
assets = []

for file in zip_assets:
    id = path = file.find('fileId').text
    path = file.find('filePackageName').text

    if not path.endswith('.zip'):
        eprint(f'WARNING: Ignoring asset \'{path}\' because it doesn\'t seem to be a zip asset.')
        count += 1
        continue

    zip_url = ASSETS_URL.format(year=args.year, path=path)

    try:
        eprint(f'\rDownloading zip assets (#{count} of #{total}) ... ', end='', flush=True)
        r = requests.get(zip_url)
    except Exception as err:
        eprint('ERROR')
        eprint('Reason: Failed to download zip asset:', err)
        sys.exit(1)

    if r.status_code != 200:
        eprint('ERROR')
        eprint('Reason: Unexpected HTTP response code:', r.status_code)
        sys.exit(1)

    sha256 = hashlib.sha256(r.content).hexdigest()

    assets.append({
        'id': id,
        'url': zip_url,
        'sha256': sha256
    })
    count += 1

assets = sorted(assets, key = lambda i: i['id'])

eprint('OK')

if args.manifest == None:
    eprint('YAML output:')
    print('    sources:')
    for asset in assets:
        print('      - type: archive')
        print('        url:', asset['url'])
        print('        sha256:', asset['sha256'])
        print('        strip-components: 2')
        if args.x:
            print('        x-checker-data:')
            print('          type:', 'html')
            print('          url:', final_url)
            print('          version-pattern:', re.escape(asset['id']) + '__([\d_]+)\.zip'),
            print('          url-template:', os.path.dirname(final_url) + '/' + re.escape(asset['id']) + '__$version.zip')
else:
    eprint(f'Updating manifest file "{args.manifest}" ... ', end='', flush=True)
    try:
        fd = open(args.manifest, 'r')
    except Exception as err:
        eprint('ERROR')
        eprint('Reason: Failed to open manifest file (reading):', err)
        sys.exit(1)

    try:
        yaml = YAML()
        yaml.indent(sequence=4, offset=2)
        manifest = yaml.load(fd)
        fd.close()
    except Exception as err:
        eprint('ERROR')
        eprint('Reason: Failed to parse manifest file as YAML:', err)
        sys.exit(1)

    for k, module in enumerate(manifest['modules']):
        if module['name'] != 'irpf_xml':
            continue
        # empty the previous sources list
        manifest['modules'][k]['sources'] = []
        # update the sources list with the new information
        for asset in assets:
            new_asset = {
                'type': 'archive',
                'url': asset['url'],
                'sha256': asset['sha256'],
                'strip-components': 2,
            }
            if args.x:
                new_asset['x-checker-data'] = {
                    'type': 'html',
                    'url': final_url,
                    'version-pattern': re.escape(asset['id']) + '__([\d_]+)\.zip',
                    'url-template': os.path.dirname(final_url) + '/' + re.escape(asset['id']) + '__$version.zip'
                }
            manifest['modules'][k]['sources'].append(new_asset)
        break

    try:
        fd = open(args.manifest, 'w')
    except Exception as err:
        eprint('ERROR')
        eprint('Reason: Failed to open manifest file (writing):', err)
        sys.exit(1)

    yaml.dump(manifest, fd)
    fd.close()
    eprint('OK')
