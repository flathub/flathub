#!/usr/bin/env python3
import sys
import yaml
import argparse

def treeish(gitsrc):
    if 'commit' in gitsrc:
        return gitsrc['commit']
    if 'tag' in gitsrc:
        return gitsrc['tag']
    if 'branch' in gitsrc:
        return gitsrc['branch']
    return 'main'

def lastnamed(items):
    name = None
    for x in items:
        if "name" in x:
            name = x["name"]
    return name


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Parse the ref from a flatpak git source")
    parser.add_argument('manifest', metavar='FILE', type=str, action='store',
        help='Flatpak manfest file to parse')
    parser.add_argument('-m', '--module', metavar='NAME', type=str,
        help='Find git ref from module NAME (default: last named module)')
    args = parser.parse_args()

    # Load the manifest
    with open(args.manifest, "r") as fp:
        modules = yaml.load(fp, Loader=yaml.SafeLoader)['modules']

    # Use the last-named module if not otherwise specified
    if not args.module:
        args.module = lastnamed(modules)

    # Find the module with the desired name.
    for mod in modules:
        if not "name" in mod:
            continue
        if mod["name"] != args.module:
            continue
        
        for src in mod["sources"]:
            if not isinstance(src, dict):
                continue
            if src['type'] != 'git':
                continue

            print(treeish(src))
            exit(0)

        print(f"No git source found in {args.module}", file=sys.stderr)
        exit(1)

    print(f"Module {args.module} not found", file=sys.stderr)
    exit(1)
