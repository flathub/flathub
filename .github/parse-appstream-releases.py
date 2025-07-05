#!/usr/bin/env python3
import sys
import xml.etree.ElementTree as ET

latest = ET.parse(sys.argv[1]).getroot().find('release')
print(f"APPSTREAM_TAG=v{latest.attrib['version']}")
for url in latest.findall('url'):
    if url.attrib['type'] == 'details':
        print(f"APPSTREAM_URL={url.text.strip()}")
