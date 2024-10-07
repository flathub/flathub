#!/usr/bin/env python3

__license__ = 'MIT'
import aiohttp
import argparse
import asyncio
import json
import hashlib
import logging
import re

arches = {
        'linux-x86_64': 'x86_64',
        'linux-x86_32': 'i386',
        'linux-aarch_64': 'aarch64',
        'linux-aarch_32': 'arm'
}

async def get_remote_sha256(url):
    logging.info(f"started sha256({url})")
    sha256 = hashlib.sha256()
    async with aiohttp.ClientSession(raise_for_status=True) as http_session:
        async with http_session.get(url) as response:
            while True:
                data = await response.content.read(4096)
                if not data:
                    break
                sha256.update(data)
    logging.info(f"done sha256({url})")
    return sha256.hexdigest()

async def parse_url(url, destdir, arch=None):
    ret = [{ 'type': 'file',
            'url': url,
            'sha256': await get_remote_sha256(url),
            'dest': destdir, }]
    if arch:
        ret[0]['only-arches'] = [arch]
    return ret

def arch_for_url(url, urls_arch):
    arch = None
    try:
        arch = urls_arch[url]
    except KeyError:
        pass
    return arch

async def parse_urls(urls, urls_arch, destdir):
    sources = []
    sha_coros = []
    for url in urls:
        arch = arch_for_url(url, urls_arch)
        sha_coros.append(parse_url(str(url), destdir, arch))
    sources.extend(sum(await asyncio.gather(*sha_coros), []))
    return sources

def gradle_arch_to_flatpak_arch(arch):
    return arches[arch]

def flatpak_arch_to_gradle_arch(arch):
    rev_arches = dict((v, k) for k, v in arches.items())
    return rev_arches[arch]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input', help='The gradle log file')
    parser.add_argument('output', help='The output JSON sources file')
    parser.add_argument('--destdir',
                        help='The directory the generated sources file will save sources to',
                        default='dependencies')
    parser.add_argument('--arches',
                        help='Comma-separated list of architectures the generated sources will be for',
                        default='x86_64,aarch64,i386,arm')
    args = parser.parse_args()
    req_flatpak_arches = args.arches.split(',')
    req_gradle_arches = []
    for arch in req_flatpak_arches:
        req_gradle_arches.append(flatpak_arch_to_gradle_arch(arch))

    urls = []
    urls_arch = {}
    r = re.compile('https://[\\w/\\-?=%.]+\\.[\\w/\\-?=%.]+')
    with open(args.input,'r') as f:
        for lines in f:
            res = r.findall(lines)
            for url in res:
                if url.endswith('.jar'):
                    urls.append(url)
                elif url.endswith('.exe'):
                    for host in req_gradle_arches:
                        if host in url:
                            for arch in req_gradle_arches:
                                new_url = url.replace(host, arch)
                                urls.append(new_url)
                                urls_arch[new_url] = gradle_arch_to_flatpak_arch(arch)

    # print(urls)
    # print(urls_arch)

    sources = asyncio.run(parse_urls(urls, urls_arch, args.destdir))

    with open(args.output, 'w') as fp:
        json.dump(sources, fp, indent=4)
        fp.write('\n')


if __name__ == '__main__':
    main()
