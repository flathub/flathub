import json
import re
from urllib import request

BINARY_BASE_URL = 'https://ftp.mozilla.org/pub/firefox/releases'
VERSIONS_API_URL = 'https://product-details.mozilla.org/1.0/firefox_versions.json'
VERSION_INFO_API_URL = 'https://product-details.mozilla.org/1.0/firefox.json'


def current_esr_version():
    with request.urlopen(VERSIONS_API_URL) as ni:
        return json.load(ni)['FIREFOX_ESR']


def build_sha256dict(version):
    sha256_dict = {}
    url = f'{BINARY_BASE_URL}/{version}/SHA256SUMS'
    with request.urlopen(url) as fi:
        text = fi.read().decode('utf-8')
        for line in text.split('\n'):
            l = re.split(r'\s+', line)
            if len(l) >= 2:
                sha256_dict[l[1]] = l[0]
    return sha256_dict


def build_language_list(version, build_number):
    url = f'https://product-details.mozilla.org/1.0/l10n/Firefox-{version}-build{build_number}.json'
    with request.urlopen(url) as fi:
        json_lang = json.load(fi)
        langs = json_lang['locales'].keys()
        return [l for l in langs if 'mac' not in l]


def get_build_info(version):
    with request.urlopen(VERSION_INFO_API_URL) as ni:
        json_obj = json.load(ni)
        return json_obj['releases'][f'firefox-{version}']


if __name__ == '__main__':
    # Tests goes here.
    version = current_esr_version()
    print(version)
    info = get_build_info(version)
    print(info)
    languages = build_language_list(version, info['build_number'])
    print(languages)
    sha256_dict = build_sha256dict(version)
    print(sha256_dict)
