#!/usr/bin/python
# -*- coding: utf-8 -*-
from datetime import datetime
import json
from urllib import request

from packaging.version import parse
from fetcher import build_language_list, build_sha256dict, current_esr_version, get_build_info

BINARY_BASE_URL = 'https://ftp.mozilla.org/pub/firefox/releases'
VERSIONS_API_URL = 'https://product-details.mozilla.org/1.0/firefox_versions.json'

platform = 'linux'
arch = 'x86_64'

def get_last_ver():
    last = ''
    try:
        f = open('./last', 'r')
        last = f.readline()
        f.close()
    except:
        pass
    return last

def set_last_ver(last):
    with open('./last', 'w') as f:
        f.write(last)

def replace_vars(input_path, output_path, **vars):
    print(input_path, output_path, vars)
    with open(input_path, 'r') as f:
        content = f.read()
        for key in vars:
            content = content.replace(f'${key}', vars[key])
        with open(output_path, 'w') as fo:
            fo.write(content)

def json_insert_modules(input_file, output_file, modules):
    with open(input_file) as fi:
        jsonInput = json.load(fi)
        for lp in modules:
            jsonInput['modules'].append(lp)
        with open(output_file, 'w') as fo:
            fo.write(json.dumps(jsonInput, indent=4))
    
def build_modules(sha256_dict, languages):
    modules = [{
        'type': 'archive',
        'url': f'{BINARY_BASE_URL}/{version}/{platform}-{arch}/en-US/firefox-{version}.tar.bz2',
        'sha256': sha256_dict[f'{platform}-{arch}/en-US/firefox-{version}.tar.bz2'],
        'only-arches': [arch],
        'dest': 'firefox'
    }]
    for lang in languages:
        info = {
            'type': 'file',
            'path': f'langpacks/{lang}.xpi',
            'url': f'https://ftp.mozilla.org/pub/firefox/releases/{version}/{platform}-{arch}/xpi/{lang}.xpi',
            'sha256': sha256_dict[f'{platform}-{arch}/xpi/{lang}.xpi'],
            'dest': 'langpacks/',
            'dest-filename': f'{lang}.xpi',
            'only-arches': [arch]
        }
        modules.append(info)
    return modules

if __name__ == '__main__':
    version = current_esr_version()
    if parse(version) <= parse(get_last_ver()):
        print('No update!')
        exit(0)
    else:
        set_last_ver(version)
    info = get_build_info(version)
    languages = build_language_list(version, info['build_number'])
    sha256_dict = build_sha256dict(version)

    modules = build_modules(sha256_dict, languages)
    json_insert_modules('./in/org.mozilla.firefox_esr.json.template', './org.mozilla.firefox_esr.json', modules)
    replace_vars('./in/org.mozilla.firefox_esr.appdata.xml.template', './org.mozilla.firefox_esr.appdata.xml', VERSION=version, RELEASE_TIMESTAMP=info['date'])