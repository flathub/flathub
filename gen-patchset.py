#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Create flatpak source from patch files")
    parser.add_argument("-o", "--output", help="Output json file")
    parser.add_argument("patchdir")
    args = parser.parse_args()

    patchdir = Path(args.patchdir)
    assert patchdir.is_dir()
    patch_paths = []
    for path in sorted(patchdir.iterdir()):
        if not path.is_file():
            continue
        if not (path.name.endswith(".patch") or path.name.endswith(".diff")):
            continue
        patch_paths.append(str(path))

    patch_source = {
        "type": "patch",
        "paths": patch_paths
    }
    if args.output:
        patch_source_file = args.output
    else:
        patch_source_file = str(patchdir).replace("/", "-") + ".json"
    with open(patch_source_file, "w") as out:
        print(patch_source_file)
        json.dump(patch_source, out, indent=4)


if __name__ == "__main__":
    main()
