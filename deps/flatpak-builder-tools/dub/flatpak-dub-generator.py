#!/usr/bin/env python3

__license__ = 'MIT'
import json
import urllib.parse
import hashlib
import logging
import argparse
import asyncio
import aiohttp

REGISTRY_URL = "https://code.dlang.org/"

async def get_remote_sha256(http_session, url):
    logging.info(f"started sha256({url})")
    sha256 = hashlib.sha256()
    async with http_session.get(url) as response:
        while True:
            data = await response.content.read(4096)
            if not data:
                break
            sha256.update(data)
    logging.info(f"done sha256({url})")
    return sha256.hexdigest()

def load_dub_selections(dub_selections_file="dub.selections.json"):
    with open(dub_selections_file, "r") as f:
        dub_selections = json.load(f)
    assert dub_selections.get("fileVersion") == 1
    return dub_selections

async def get_sources(http_session, name, version_obj):
    if isinstance(version_obj, dict):
        if "path" in version_obj:
            logging.warning(f"Skipping path based dependency {name}")
            return
        version = version_obj["version"]
    else:
        version = version_obj
    dl_url = urllib.parse.urljoin(REGISTRY_URL, f"/packages/{name}/{version}.zip")
    source = {
        "type": "archive",
        "url": dl_url,
        "sha256": await get_remote_sha256(http_session, dl_url),
        "dest": f".flatpak-dub/{name}-{version}"
    }
    local_package = {
        "name": name,
        "version": version,
        "path": f".flatpak-dub/{name}-{version}"
    }
    return (source, local_package)

async def generate_sources(dub_selections):
    sources = []
    local_packages = []

    async with aiohttp.ClientSession() as http_session:
        coros = []
        for name, version_obj in dub_selections["versions"].items():
            coros.append(get_sources(http_session, name, version_obj))
        dub_sources = await asyncio.gather(*coros)
        for dub_source in dub_sources:
            if dub_source is not None:
                source, local_package = dub_source
                sources.append(source)
                local_packages.append(local_package)
    sources += [
        {
            "type": "inline",
            "contents": json.dumps(local_packages),
            "dest": ".dub/packages",
            "dest-filename": "local-packages.json"
        },
        {
            "type": "shell",
            "commands": [
                (
                    "jq 'map(.path = ([$ENV.PWD] + (.path | split(\"/\")) | join(\"/\")))' "
                    "<<<$(<.dub/packages/local-packages.json) > .dub/packages/local-packages.json"
                )
            ]
        }
    ]

    return sources

async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('dub_selections_file', help='Path to the dub.selections.json file')
    parser.add_argument('-o', '--output', required=False, help='Where to write generated sources')
    args = parser.parse_args()
    if args.output is not None:
        outfile = args.output
    else:
        outfile = 'generated-sources.json'

    generated_sources = await generate_sources(load_dub_selections(args.dub_selections_file))
    with open(outfile, 'w') as out:
        json.dump(generated_sources, out, indent=4, sort_keys=False)

if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    asyncio.run(main())
