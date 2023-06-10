#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import argparse
import sys

from packaging.version import parse
from fetcher import build_language_list, build_sha256dict, current_esr_version, get_build_info, BINARY_BASE_URL


platform = 'linux'
arches = ['x86_64']
freedesktop_version = '22.08'


class MyJsonTapper:
    json_data = {}

    def __init__(self, file_path) -> None:
        with open(file_path, 'r', encoding='utf-8') as fi:
            self.json_data = json.load(fi)

    def tap(self, func, *args):
        func(self.json_data, *args)
        return self

    def output(self, output_path):
        with open(output_path, 'w', encoding='utf-8') as fo:
            fo.write(json.dumps(self.json_data, indent=4))


def get_last_ver():
    last = '0.0'
    try:
        f = open('./last', 'r', encoding='utf-8')
        last = f.readline()
        f.close()
    except:
        pass
    return last


def set_last_ver(last):
    with open('./last', 'w', encoding='utf-8') as f:
        f.write(last)


def get_matching_freedesktop_version():
    with open('./BaseApp/org.mozilla.firefox.BaseApp.json', 'r', encoding='utf-8') as fi:
        json_baseapp = json.load(fi)
        return json_baseapp['runtime-version']


def varsubst(input_path, output_path, **vars):
    print(input_path, output_path, vars)
    with open(input_path, 'r', encoding='utf-8') as f:
        content = f.read()
        for key in vars:
            content = content.replace(f'${key}', vars[key])
        with open(output_path, 'w', encoding='utf-8') as fo:
            fo.write(content)


def extend_firefox_source(json_object, sha256_dict, languages, version):
    sources_to_extend = []
    for arch in arches:
        sources_to_extend.append({
            'type': 'archive',
            'url': f'{BINARY_BASE_URL}/{version}/{platform}-{arch}/en-US/firefox-{version}.tar.bz2',
            'sha256': sha256_dict[f'{platform}-{arch}/en-US/firefox-{version}.tar.bz2'],
            'only-arches': [arch],
            'dest': 'firefox'
        })
        for lang in languages:
            sources_to_extend.append({
                'type': 'file',
                'path': f'langpacks/{lang}.xpi',
                'url': f'https://ftp.mozilla.org/pub/firefox/releases/{version}/{platform}-{arch}/xpi/{lang}.xpi',
                'sha256': sha256_dict[f'{platform}-{arch}/xpi/{lang}.xpi'],
                'dest': 'langpacks/',
                'dest-filename': f'{lang}.xpi',
                'only-arches': [arch]
            })
    for item in json_object['modules']:
        if isinstance(item, dict) and item['name'] == 'firefox':
            item['sources'].extend(sources_to_extend)
            break


def set_freedesktop_version(json_objects, version):
    json_objects['runtime-version'] = str(version)
    extension_root = json_objects['add-extensions']
    for k in extension_root:
        extension_root[k]['version'] = version


def merge_baseapp(json_objects, channel):
    json_objects['base-version'] = channel


def write_output_kv(file, new_version):
    with open(file, 'w', encoding='utf-8') as f:
        try:
            f.write(f'new_version={new_version}')
        except:
            print('Cannot write to file', file, file=sys.stderr);


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--force_version', type=str,
                        help="Force version", nargs='?')
    parser.add_argument('--output_kv', type=str,
                        help='Output result as a key-value pair to a file.', nargs='?')
    parser.add_argument('--skip_check', action='store_true',
                        help='Skip version check', default=False)
    args = parser.parse_args()

    if args.force_version is not None:
        print("Forcing version {}".format(args.force_version))
        version_str = args.force_version
    else:
        version_str = current_esr_version()
    if (not args.skip_check) and parse(version_str) <= parse(get_last_ver()):
        print('No update!')
        if args.output_kv is not None:
            write_output_kv(args.output_kv, 'na')
        exit(0)
    else:
        set_last_ver(version_str)

    freedesktop_version = get_matching_freedesktop_version()
    info = get_build_info(version_str)
    languages = build_language_list(version_str, info['build_number'])
    sha256_dict = build_sha256dict(version_str)

    MyJsonTapper('./in/org.mozilla.firefox_esr.json.template').tap(extend_firefox_source,
                                                                   sha256_dict, languages, version_str).tap(merge_baseapp, freedesktop_version).tap(set_freedesktop_version, freedesktop_version).output('./org.mozilla.firefox_esr.json')
    varsubst('./in/org.mozilla.firefox_esr.appdata.xml.template',
             './org.mozilla.firefox_esr.appdata.xml', VERSION=version_str, RELEASE_TIMESTAMP=info['date'])
    if args.output_kv is not None:
        write_output_kv(args.output_kv, version_str)
    exit(0)
