#!/usr/bin/env python3
import os
import json
import subprocess
import urllib.parse
import argparse

def inspect(image_spec: str):
    proc = subprocess.run(['skopeo', 'inspect', image_spec],
                          check=True, stdout=subprocess.PIPE)
    result = json.loads(proc.stdout)
    return result

def make_index(registry: str, images: list):
    index = {'Registry': registry, 'Results': []}
    for img_spec in images:
        image_url = urllib.parse.urlparse(urllib.parse.urljoin(registry, img_spec))
        index['Results'].append({
            'Name': img_spec,
            'Images': [
                inspect(image_url._replace(scheme='docker').geturl())
            ]
        })
    return index

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--registry', required=True)
    parser.add_argument('image', nargs='+')
    args = parser.parse_args()
    print(json.dumps(make_index(args.registry, args.image), indent=4))

if __name__ == '__main__':
    main()
