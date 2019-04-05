#!/usr/bin/env python3

import os
import sys
import subprocess
from datetime import datetime
from lxml import etree
from ruamel.yaml import YAML

MANIFEST = 'com.visualstudio.code.yaml'
APPDATA = 'com.visualstudio.code.appdata.xml'

GIT = ['git', 'ls-remote', '--tags', '--sort=-v:refname', 'https://github.com/Microsoft/vscode', '?.??.?']
VERSION = subprocess.check_output(GIT).decode("utf-8").splitlines()[0].split('/')[2]

yaml = YAML()
yaml.indent(mapping=2, sequence=4, offset=2)
yaml.width = 200

with open(MANIFEST, 'r') as f:
    data = yaml.load(f)

if VERSION in data['modules'][-1]['sources'][-1]['url']:
    print('No update needed. Current version: ' + VERSION)
    sys.exit()

ARCHES = {'x64': -2, 'ia32': -1}

for arch, pos in ARCHES.items():
    source_entry = data['modules'][-1]['sources'][pos]
    source_entry['url'] = 'https://vscode-update.azurewebsites.net/' + VERSION + '/linux-deb-' + arch + '/stable'
    FILENAME = 'code_' + VERSION + '_' + arch + '.deb'
    subprocess.call(['curl', '-R', '-L', '-o', FILENAME, '-C', '-', source_entry['url']])
    source_entry['sha256'] = subprocess.check_output(['sha256sum', FILENAME]).decode("utf-8").split(None, 1)[0]
    source_entry['size'] = os.path.getsize(FILENAME)

with open(MANIFEST, 'w') as f:
    yaml.dump(data, f)

release = etree.Element('release', {
    'version': VERSION,
    'date': datetime.utcfromtimestamp(os.path.getmtime(FILENAME)).strftime('%Y-%m-%d')
})
parser = etree.XMLParser(remove_comments=False)
tree = etree.parse(APPDATA, parser=parser)
releases = tree.find('releases')
for child in list(releases):
    releases.remove(child)
release.tail = '\n  '
releases.append(release)
tree.write(APPDATA, encoding="utf-8", xml_declaration=True)

print("Update done. New version: " + VERSION)
