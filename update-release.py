#!/usr/bin/env python3

from datetime import datetime
import json
import re
import sys
import urllib.request

from lxml import etree

PACKAGE_URL = 'https://repository-origin.spotify.com/dists/testing/non-free/binary-amd64/Packages'
REPO_URL = 'http://repository.spotify.com/'
MANIFEST = 'com.spotify.EdgeClient.json'
APPDATA = 'com.spotify.EdgeClient.appdata.xml'


with urllib.request.urlopen(PACKAGE_URL) as conn:
    data = conn.read().decode()

    sha256 = re.findall(r'^SHA256: (.*)', data, re.M)[0]
    filename = re.findall(r'^Filename: (.*)', data, re.M)[0]
    size = int(re.findall(r'^Size: (.*)', data, re.M)[0])
    url = REPO_URL + filename
    version = re.findall(r'(\d+\.\d+\.\d+\.\d+)', filename)[0]


with open(MANIFEST, 'r') as f:
    data = json.load(f)
    source_entry = data['modules'][-1]['sources'][-2]

    if source_entry['url'] == url:
        print('Already up to date')
        sys.exit()

    source_entry['url'] = url
    source_entry['sha256'] = sha256
    source_entry['size'] = size


with open(MANIFEST, 'w') as f:
    json.dump(data, f, indent=4)
    print('Updated manifest, please test')


parser = etree.XMLParser(remove_comments=False)
tree = etree.parse(APPDATA, parser=parser)
release = etree.Element('release', {
    'version': version,
    'date': datetime.today().strftime('%Y-%m-%d')  # Close enough
})
releases = tree.find('releases')
release.tail = '\n    '
releases.insert(0, release)
tree.write(APPDATA)
print('Updated appdata, ensure its correct')
